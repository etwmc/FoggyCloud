package main

/*
#include "process.h"
*/
import (
	"C"
)
import (
	"bytes"
	"encoding/binary"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"html"
	"log"
	"math"
	"math/rand"
	"net"
	"net/http"
	"os"
	"strconv"
	"sync"
	"time"
	"unsafe"
)

//Constant
var bucketSize = 5

var portNum = "8080"

//Opening project
var muxList = map[string](*mlModelPara){}

var clients = make([]string, 1)

type countBucket struct {
	Buffer   []float64
	MaxCount int
	Count    int
	Parent   *countBucket
}

type dataBucket struct {
	Weights     []int
	Buffer      []float64
	ValueLength int
	MaxCount    int
	Count       int
	Parent      *dataBucket
	m           sync.Mutex
}

func (d *dataBucket) grow() {
	if d.Count == 1 {
		return
	}
	if d.Parent == nil {
		d.Parent = new(dataBucket)
		d.Parent.init(d.ValueLength, d.MaxCount)
	}
	value, count := d.process()
	d.Parent.addBuffer(value, count)
	//After grow, flush
	d.Count = 0
}

func (d *dataBucket) process() ([]float64, int) {
	totalWeight := C.counter(unsafe.Pointer(&d.Weights[0]), C.longlong(d.Count))
	result := make([]float64, d.ValueLength)
	C.avgSum(unsafe.Pointer(&d.Weights[0]), unsafe.Pointer(&d.Buffer[0]), C.longlong(d.ValueLength), C.longlong(d.Count), unsafe.Pointer(&result[0]))
	return result, int(totalWeight)
}

func (d *dataBucket) addBuffer(v []float64, weight int) {
	//Verify input
	d.m.Lock()
	copy(d.Buffer[d.Count*d.ValueLength:(d.Count+1)*d.ValueLength], v)
	d.Weights[d.Count] = weight
	d.Count++
	if d.MaxCount <= d.Count {
		d.grow()
	}
	d.m.Unlock()
}

func (d *dataBucket) finalValue() ([]float64, int) {
	d.m.Lock()
	defer d.m.Unlock()
	//Finalize remaining content
	if d.Count > 0 {
		d.grow()
	}
	if d.Parent != nil {
		return d.Parent.finalValue()
	} else {
		tmp := make([]float64, d.ValueLength)
		copy(tmp, d.Buffer[0:d.ValueLength])
		for index := 0; index < d.ValueLength; index++ {
			tmp[index] /= float64(d.Weights[0])
		}

		return tmp, d.Weights[0]
	}
}

func (d *dataBucket) init(valueLen, size int) {
	d.Weights = make([]int, size)
	d.Buffer = make([]float64, size*valueLen)
	d.ValueLength = valueLen
	d.MaxCount = size
	d.Count = 0
	d.Parent = nil
}

type dataBucketMux struct {
	Name     string
	DataLen  int
	Buckets  [](*dataBucket)
	Duration int
	Version  int
	ddlTimer *time.Timer
}

func createBucketMux(muxName string, version int, dataLen, numberOfBucket, duration int) *dataBucketMux {

	mux := new(dataBucketMux)

	mux.Name = muxName
	mux.DataLen = dataLen

	mux.Buckets = make([]*dataBucket, numberOfBucket)
	for i := 0; i < numberOfBucket; i++ {
		mux.Buckets[i] = new(dataBucket)
		(*mux).Buckets[i].init(dataLen, bucketSize)
	}
	mux.Duration = duration
	if duration > 0 {
		mux.ddlTimer = time.AfterFunc(time.Duration(duration)*time.Second, func() {
			takingLead(muxName, version, clients)
		})
	}
	return mux
}

func (dbm *dataBucketMux) bucketRequest(value []float64, weight int, ticket int) {
	db := dbm.Buckets[ticket]
	db.addBuffer(value, weight)
}

func stopUpdating(dbm *dataBucketMux) {

}

func (dbm *dataBucketMux) finalResult() ([]float64, int) {
	//Create a bucket to mux the result
	bucket := new(dataBucket)
	bucket.init(dbm.DataLen, len(dbm.Buckets))
	gp := new(sync.WaitGroup)
	gp.Add(len(dbm.Buckets))
	for i := 0; i < len(dbm.Buckets); i++ {
		target := dbm.Buckets[i]
		go func() {
			value, weight := target.finalValue()
			bucket.addBuffer(value, weight)
			gp.Done()
		}()
	}

	gp.Wait()
	return bucket.finalValue()
}

func hashToToken(hash string, numberMachine int) int {
	//Strip the last
	val, err := hex.DecodeString(hash)
	var index uint64
	if err != nil {
		index = binary.LittleEndian.Uint64(val)
	}
	if err != nil {
		return rand.Intn(numberMachine)
	} else {
		return int(index) % numberMachine
	}
}

func authenticatedLAN(addr string) bool {
	ip, port, err := net.SplitHostPort(addr)
	print(ip, port)
	//Fallback
	if err != nil {
		return false
	}
	return true
}

func takingLead(projectName string, version int, loadBalance []string) {
	//Get the current mux
	model := muxList[projectName]

	//Create a bucket
	var bucket dataBucket
	bucket.init(model.ParaLen, 5)

	var gp sync.WaitGroup
	gp.Add(len(loadBalance))

	for _, addr := range loadBalance {
		go func(addr string) {
			resp, err := http.Get("http://" + addr + "/Result")
			if err != nil {
				println("Error, can't connect to terminal ", addr)
			} else {
				//Get the package, so process
				if sampleNum := resp.Header.Get("SampleCount"); len(sampleNum) > 0 {
					if numInstance, err := strconv.Atoi(sampleNum); err == nil {
						buffer := make([]float64, model.ParaLen)
						bucket.addBuffer(buffer, numInstance)
					}
				}
			}
			gp.Done()
		}(addr)
	}

	gp.Wait()

	delta, sampleCount := bucket.finalValue()

	if sampleCount > 0 {
		//There is some learning
		file, err := os.Create("projectName." + strconv.Itoa(version) + ".delta") // For read access.
		if err != nil {
			log.Fatal(err)
		}

		err = binary.Write(file, binary.LittleEndian, delta)
		if err != nil {
			log.Fatal(err)
		} else {
			log.Println("Sample Count", sampleCount)
			file.Close()
		}

		model.TrainingMux.Version++
	}

}

type mlModelPara struct {
	ModelName    string
	TrainingMux  *dataBucketMux
	ModelPara    []float64
	ParaLen      int
	Version      int
	LearningRate float64
	RWLock       sync.RWMutex
}

func modelParaConf(name string, paraLen int, secondPerSession int) *mlModelPara {
	para := new(mlModelPara)
	para.ModelName = name
	para.ParaLen = paraLen
	para.ModelPara = make([]float64, paraLen)
	para.TrainingMux = createBucketMux(name, 1, paraLen, 5, secondPerSession)
	para.Version = 1
	return para
}

func logMux(_ *dataBucketMux, _ int) {

}

func (para *mlModelPara) modelParaNewVersion(delta []float64) {
	newPara := make([]float64, para.ParaLen)
	C.vecDScaleThenAdd(unsafe.Pointer(&para.ModelPara[0]), unsafe.Pointer(&delta[0]), C.double(para.LearningRate), unsafe.Pointer(&newPara[0]), C.longlong(para.ParaLen))
	//Replacement
	para.RWLock.Lock()
	para.Version++
	para.ModelPara = newPara
	//Since the para are new, reset the training mux
	oldVersion := para.Version - 1
	oldTrainingMux := para.TrainingMux
	para.TrainingMux = createBucketMux(oldTrainingMux.Name, 1, para.ParaLen, 5, oldTrainingMux.Duration)
	para.RWLock.Unlock()
	//Write back
	logMux(oldTrainingMux, oldVersion)
}

func pickModel(modelName string) *mlModelPara {
	return muxList[modelName]
}

func main() {

	//The project that it takes an lead
	//muxLead := []string
	//loadBalance := []string

	rand.Seed(time.Now().UnixNano())
	clients[0] = "localhost:" + portNum

	dataLen := 10
	numberOfMach := 1

	conf := modelParaConf("Test", dataLen, 60)

	muxList["Test"] = conf

	//Gather reports
	http.HandleFunc("/ML", func(w http.ResponseWriter, r *http.Request) {

		//Version string check
		versionStr := r.Header.Get("Version")
		//No version, early leave
		if len(versionStr) == 0 {
			w.WriteHeader(404)
			return
		}
		var version int
		var err error
		if version, err = strconv.Atoi(versionStr); err != nil {
			w.WriteHeader(404)
			return
		}

		var model *mlModelPara
		if modelName := r.Header.Get("Model"); len(modelName) > 0 {
			model = pickModel(modelName)
			if model == nil {
				w.WriteHeader(404)
				return
			}
		} else {
			w.WriteHeader(404)
			return
		}

		model.RWLock.RLock()

		if model.Version != version {
			//Old Version
			model.RWLock.RUnlock()
			w.WriteHeader(404)
			return
		}

		//Copy variable needed
		mux := model.TrainingMux

		model.RWLock.RUnlock()

		if sampleNum := r.Header.Get("SampleCount"); len(sampleNum) > 0 {
			//Get values
			data := make([]float64, dataLen)
			if _, err := r.MultipartReader(); err == nil {
				//It is a multi part data
				fmt.Fprintf(w, "Error")
				return
			} else {
				valueReader := r.Body
				buffer := make([]byte, dataLen*8)
				n, _ := valueReader.Read(buffer)

				if n == dataLen*8 {
					for index := 0; index < dataLen; index++ {
						bits := binary.LittleEndian.Uint64(buffer[index*8 : index*8+8])
						data[index] = math.Float64frombits(bits)
					}
				} else {
					return
				}
			}
			idHash := r.Header.Get("Hash")
			ticket := hashToToken(idHash, numberOfMach)
			if numInstance, err := strconv.Atoi(sampleNum); err == nil {
				go mux.bucketRequest(data, numInstance, ticket)
				fmt.Fprintf(w, "Hello, %q", html.EscapeString(r.URL.Path))
			} else {
				log.Fatal(err)
				fmt.Fprintf(w, "Error: %s", err.Error())
			}
		}
	})

	//Report the partial delta on this node
	http.HandleFunc("/Result", func(w http.ResponseWriter, r *http.Request) {

		//Version string check
		versionStr := r.Header.Get("Version")
		//No version, early leave
		if len(versionStr) == 0 {
			w.WriteHeader(404)
			return
		}
		var version int
		var err error
		if version, err = strconv.Atoi(versionStr); err != nil {
			w.WriteHeader(404)
			return
		}

		var model *mlModelPara
		if modelName := r.Header.Get("Model"); len(modelName) > 0 {
			model = pickModel(modelName)
			if model == nil {
				w.WriteHeader(404)
				return
			}
		} else {
			w.WriteHeader(404)
			return
		}

		model.RWLock.RLock()

		if model.Version != version {
			//Old Version
			model.RWLock.RUnlock()
			w.WriteHeader(404)
			return
		}

		//Copy variable needed
		mux := model.TrainingMux

		model.RWLock.RUnlock()

		//Sumbit delegate result

		values, count := mux.finalResult()
		buffer := new(bytes.Buffer)
		err = binary.Write(buffer, binary.LittleEndian, values)
		w.Header().Set("SampleCount", string(count))
		if err != nil {
			w.WriteHeader(500)
		} else {
			w.Write(buffer.Bytes())
		}
	})

	//Access the lastest version of the model
	http.HandleFunc("/Model", func(w http.ResponseWriter, r *http.Request) {
		if modelName := r.Header.Get("Model"); len(modelName) > 0 {
			model := pickModel(modelName)
			if model == nil {
				w.WriteHeader(404)
				return
			}
			model.RWLock.RLock()
			values := model.ModelPara
			versions := model.Version
			model.RWLock.RUnlock()
			buffer := new(bytes.Buffer)
			err := binary.Write(buffer, binary.LittleEndian, values)
			w.Header().Set("Version", string(versions))
			if err != nil {
				w.WriteHeader(500)
			} else {
				w.Write(buffer.Bytes())
			}
		}
	})

	//Debug interface
	http.HandleFunc("/Dump", func(w http.ResponseWriter, r *http.Request) {

		//Version string check
		versionStr := r.Header.Get("Version")
		//No version, early leave
		if len(versionStr) == 0 {
			w.WriteHeader(404)
			return
		}
		var version int
		var err error
		if version, err = strconv.Atoi(versionStr); err != nil {
			w.WriteHeader(404)
			return
		}

		var model *mlModelPara
		if modelName := r.Header.Get("Model"); len(modelName) > 0 {
			model = pickModel(modelName)
			if model == nil {
				w.WriteHeader(404)
				return
			}
		} else {
			w.WriteHeader(404)
			return
		}

		model.RWLock.RLock()

		if model.Version != version {
			//Old Version
			model.RWLock.RUnlock()
			w.WriteHeader(404)
			return
		}

		//Copy variable needed
		mux := model.TrainingMux

		model.RWLock.RUnlock()

		b, err := json.Marshal(mux)
		if err != nil {
			fmt.Fprintf(w, err.Error())
			return
		}
		fmt.Fprintf(w, string(b))
	})

	log.Fatal(http.ListenAndServe(":"+portNum, nil))
}
