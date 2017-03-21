//
//  DeltaPager.c
//  
//
//  Created by Wai Man Chan on 12/1/16.
//
//

#include "DeltaPager.h"
#include <string.h>
#include <sqlite3.h>
#include "RandomCipher.h"
#include "CVtKeyManagment.h"
#include "CloudVault.h"
#include "DeltaPager_Priv.h"
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>

//VFS functions
#define underlyingVFS ((encrypedVFS*)vfs)->underneathVFS
#define underlyingFile ((encryptFile*)file)->actualFile
#define modifingFile ((encryptFile*)file)->modifyFile

extern sqlite3_io_methods encryptedMethod;

bool fileIsTemp(const char *zName) {
    size_t len = strlen(zName);
    if (zName[len-4] == '-' && zName[len-3] == 'w' && zName[len-2] == 'a' && zName[len-1] == 'l') return true;
    if (zName[len-8] == '-' && zName[len-7] == 'j' && zName[len-6] == 'o' && zName[len-5] == 'u'&&
        zName[len-4] == 'r' && zName[len-3] == 'n' && zName[len-2] == 'a' && zName[len-1] == 'l') return true;
    return false;
}

#define USE_MMAP 1

//Mapping service
void loadBitmap(encryptFile *file) {
    //Get actual size, so map
    //Because of page schema, the file size will be an integer of the (blockSize * 8)
    
#if USE_MMAP
    
    int fd = fileno(file->bitMapFile);
    size_t len = file->actualFileSize/16;
    ftruncate(fd, len);
    
    file->modifiedBit = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    
#else
    
    fseek(file->bitMapFile, 0, SEEK_SET);
    size_t ret = fread(file->modifiedBit, sizeof(bool), file->actualFileSize/16, file->bitMapFile);
    if (ret != file->actualFileSize) {
        //It means during the last edit, we growed the original text after the
        for (int i = ret; i < file->actualFileSize/16; i++)
            file->modifiedBit[i] = false;
    }
#endif
    file->bitDirty = false;
}

void saveBitmap(encryptFile *file) {
    //Get actual size, so map
    //Because of page schema, the file size will be an integer of the (blockSize * 8)
#if USE_MMAP
    int err = msync(file->modifiedBit, file->actualFileSize/16, MS_SYNC);
#else
    if (file->bitDirty) {
        fseek(file->bitMapFile, 0, SEEK_SET);
        fwrite(file->modifiedBit, sizeof(bool), file->actualFileSize/16, file->bitMapFile);
        fflush(file->bitMapFile);
        file->bitDirty = false;
    }
#endif
    fflush(file->bitMapFile);
}

void growBitmap(encryptFile *file, sqlite_int64 newSize) {
    if (ftruncate(fileno(file->bitMapFile), newSize/16) != 0) {
        file->bitDirty = true;
    }
#if USE_MMAP
    msync(file->modifiedBit, file->actualFileSize/16, MS_SYNC);
    munmap(file->modifiedBit, file->actualFileSize/16);
    //Reload file
    fclose(file->bitMapFile);
    char newAddr[4096];
    bcopy(file->addr, newAddr, strlen(file->addr));
    bcopy(".map", newAddr+strlen(file->addr), 5);
    file->bitMapFile = fopen(newAddr, "r+");
    int fd = fileno(file->bitMapFile);
    size_t len = newSize/16;
    file->modifiedBit = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
#else
    bool *newVector = malloc(sizeof(bool)*newSize/16);
    bcopy(file->modifiedBit, newVector, file->actualFileSize/16);
    for (sqlite_int64 j = file->actualFileSize/16; j < newSize/16; j++) {
        newVector[j] = false;
    }
    free(file->modifiedBit);
    file->modifiedBit = newVector;
#endif
}

void updateBitmap(encryptFile *file, sqlite_int64 offset, int amount) {
    //Fill the section
    sqlite_int64 offsetDiv4 = offset/16;
    int amountDiv4 = amount/16;
    bool *bit = file->modifiedBit;
    for (int i = 0; i < amountDiv4; i++) {
        if (!file->bitDirty && bit[i+offsetDiv4] != true) file->bitDirty = true;
        bit[i+offsetDiv4] = true;
    }
}

void closeBitmap(encryptFile *file) {
    //Fill the section
    bool *bit = file->modifiedBit;
#if USE_MMAP
    munmap(file->modifiedBit, file->actualFileSize/16);
#else
    free(bit);
#endif
}

//Put the original ciphertext (unmodified) back to the ciphertext(readed ciphertext)
void mergeCiphertext(encryptFile *file, char *ciphertext, const char *original, sqlite_int64 offset, int length) {
    assert(offset % 16 == 0);
    assert(length % 16 == 0);
    int beginPt = (offset)/16;
    bool *begin = &(file->modifiedBit[beginPt]);
    
    for (int i = 0; i < length; i+=16, begin++) {
        //For each flag no, it means we need to roll back to the original
        if (*begin == false) {
            //Roll back a block
            for (int j = 0; j < 16; j++) {
                ciphertext[i+j] = original[i+j];
            }
        }
    }
}

int encryptOpen(sqlite3_vfs* vfs, const char *zName, sqlite3_file* file, int flags, int *pOutFlags) {
    //Open the underlying one
    int size = underlyingVFS->szOsFile;
    sqlite3_file *rootFile = (sqlite3_file *)malloc(size);
    sqlite3_file *deltaFile = (sqlite3_file*)malloc(size);
    bzero(rootFile, size);
    
    //Initial underneath vfs
    int resNo = underlyingVFS->xOpen(underlyingVFS, zName, rootFile, flags, pOutFlags);
    
    if (resNo == SQLITE_OK) {
        //Sucess, so continuous initial
        ((encryptFile*)file)->actualFile = rootFile;
        rootFile->pMethods->xFileSize(rootFile, &((encryptFile*)file)->actualFileSize);
        ((encryptFile*)file)->method = &encryptedMethod;
        ((encryptFile*)file)->pVfs = vfs;
    }
    
    //Copy name
    strncpy(((encryptFile*)file)->addr, zName, 4096);
    
    //Test file type
    ((encryptFile*)file)->passThr = fileIsTemp(zName);
    if ( ((encryptFile*)file)->passThr ) return resNo;
    
    //New address: zName.delta
    int newAddrLen = strlen(zName)+7;
    char *newAddr = malloc(sizeof(char)*newAddrLen);
    bcopy(zName, newAddr, strlen(zName));
    bcopy(".delta", newAddr+strlen(zName), 7);
    
    //Initial delta
    resNo |= underlyingVFS->xOpen(underlyingVFS, newAddr, deltaFile, flags, pOutFlags);
    if (resNo == SQLITE_OK) {
        
        //Get actual size, so map
        ((encryptFile*)file)->modifiedBit = malloc(sizeof(bool)*((encryptFile*)file)->actualFileSize/4);
        bzero(((encryptFile*)file)->modifiedBit, ((encryptFile*)file)->actualFileSize/4);
        bcopy(".map", newAddr+strlen(zName), 5);
        FILE *f = fopen(newAddr, "r+");
        if (f == NULL) {
            //Init file
            f = fopen(newAddr, "w");
            fclose(f);
            f = fopen(newAddr, "r+");
        }
        ((encryptFile*)file)->bitMapFile = f;
        loadBitmap(file);
        //f = fopen(newAddr, "r+");
        //((encryptFile*)file)->bitMapFile = f;
        //saveBitmap(file);
        
        ((encryptFile*)file)->modifyFile = deltaFile;
        
        //Init key
        ((encryptFile*)file)->keySize = 32;
        ((encryptFile*)file)->key = malloc(sizeof(char)*32);
        //Check key bag
        if (keybagExist(zName) == false) {
            printf("Generate key bag\n");
            generateKeyBag(zName);
        }
        copyMasterKey_self(zName, ((encryptFile*)file)->key);
    }
    free(newAddr);
    return resNo;
}
int encryptDelete(sqlite3_vfs* vfs, const char *zName, int syncDir) {
    //Delete file
    int state = underlyingVFS->xDelete(underlyingVFS, zName, syncDir);
    
    //If temp file, no delta or map
    if ( fileIsTemp(zName) ) return state;
    
    //Delete delta file
    int newAddrLen = strlen(zName)+7;
    char *newAddr = malloc(sizeof(char)*newAddrLen);
    bcopy(zName, newAddr, strlen(zName));
    bcopy(".delta", newAddr+strlen(zName), 7);
    state |= underlyingVFS->xDelete(underlyingVFS, newAddr, syncDir);
    //Delete map file
    bcopy(".map", newAddr+strlen(zName), 5);
    state |= underlyingVFS->xDelete(underlyingVFS, newAddr, syncDir);
    free(newAddr);
    return state;
}
int encryptAccess(sqlite3_vfs* vfs, const char *zName, int flags, int *pResOut) { return underlyingVFS->xAccess(underlyingVFS, zName, flags, pResOut); }
int encryptFullPathname(sqlite3_vfs* vfs, const char *zName, int nOut, char *zOut) { return underlyingVFS->xFullPathname(underlyingVFS, zName, nOut, zOut); }
void *encryptDlOpen(sqlite3_vfs* vfs, const char *zFilename) { return underlyingVFS->xDlOpen(underlyingVFS, zFilename); }
void encryptDlError(sqlite3_vfs* vfs, int nByte, char *zErrMsg) { return underlyingVFS->xDlError(underlyingVFS, nByte, zErrMsg); }
void (*encryptDlSym(sqlite3_vfs *vfs, void *p, const char*zSym))(void) { return underlyingVFS->xDlSym(underlyingVFS, p, zSym); }
void encryptDlClose(sqlite3_vfs* vfs, void*ptr) { return underlyingVFS->xDlClose(underlyingVFS, ptr); }
int encryptRandomness(sqlite3_vfs* vfs, int nByte, char *zOut) { return underlyingVFS->xRandomness(underlyingVFS, nByte, zOut); }
int encryptSleep(sqlite3_vfs* vfs, int microseconds) { return underlyingVFS->xSleep(underlyingVFS, microseconds); }
int encryptCurrentTime(sqlite3_vfs* vfs, double*ptr) { return underlyingVFS->xCurrentTime(underlyingVFS, ptr); }
int encryptGetLastError(sqlite3_vfs* vfs, int size, char *buffer) { return underlyingVFS->xGetLastError(underlyingVFS, size, buffer); }
int encryptCurrentTimeInt64(sqlite3_vfs* vfs, sqlite3_int64*ptr) { return underlyingVFS->xCurrentTimeInt64(underlyingVFS, ptr); }
int encryptSetSystemCall(sqlite3_vfs* vfs, const char *zName, sqlite3_syscall_ptr ptr) { return underlyingVFS->xSetSystemCall(underlyingVFS, zName, ptr); }
sqlite3_syscall_ptr encryptGetSystemCall(sqlite3_vfs* vfs, const char *zName) { return underlyingVFS->xGetSystemCall(underlyingVFS, zName); }
const char *encryptNextSystemCall(sqlite3_vfs* vfs, const char *zName) { return underlyingVFS->xNextSystemCall(underlyingVFS, zName); }

#define replicateVFSFuncPtr(funcName) cloneVFS->superclass.x##funcName = (originalVFS->x##funcName)? &encrypt##funcName: NULL;
#define replicateFileFuncPtr(funcName) cloneFile->fileMethod.x##funcName = (originalVFS->x##funcName)? &encrypt##funcName: NULL;

#define encryptVFS(VFSNAME, FINDER) {                        \
3,                    /* iVersion */                    \
sizeof(encryptFile),     /* szOsFile */                    \
MAX_PATHNAME,         /* mxPathname */                  \
0,                    /* pNext */                       \
VFSNAME,              /* zName */                       \
(void*)&FINDER,       /* pAppData */                    \
encryptOpen,             /* xOpen */                       \
encryptDelete,           /* xDelete */                     \
encryptAccess,           /* xAccess */                     \
encryptFullPathname,     /* xFullPathname */               \
encryptDlOpen,           /* xDlOpen */                     \
encryptDlError,          /* xDlError */                    \
encryptDlSym,            /* xDlSym */                      \
encryptDlClose,          /* xDlClose */                    \
encryptRandomness,       /* xRandomness */                 \
encryptSleep,            /* xSleep */                      \
encryptCurrentTime,      /* xCurrentTime */                \
encryptGetLastError,     /* xGetLastError */               \
encryptCurrentTimeInt64, /* xCurrentTimeInt64 */           \
encryptSetSystemCall,    /* xSetSystemCall */              \
encryptGetSystemCall,    /* xGetSystemCall */              \
encryptNextSystemCall,   /* xNextSystemCall */             \
}

//Cipher callback
#define min(a,b) a<b? a: b
#define max(a,b) a>b? a: b
int _randomRead(void *file, char *buffer, unsigned long long offest, unsigned int bufferSize, unsigned int *readSize) {
    int state = modifingFile->pMethods->xRead(modifingFile, (void*)buffer, bufferSize, offest);
    char *original = malloc(sizeof(char)*bufferSize);
    state |= underlyingFile->pMethods->xRead(underlyingFile, (void*)original, bufferSize, offest);
    sqlite_int64 fileSize = ((encryptFile*) file)->actualFileSize;
    sqlite_int64 readBufferSize = max(0, fileSize-offest);
    
    if (readBufferSize <= 0 ) {
        bzero(buffer, bufferSize);
        *readSize = 0;
        return state;
    }
    
    if (state == SQLITE_IOERR_SHORT_READ) {
        //Short read, so get the read size
        sqlite_int64 size;
        ((sqlite3_file*)file)->pMethods->xFileSize(file, &size);
        *readSize = (size-offest);
    } else *readSize = bufferSize;
    
    if (state == SQLITE_OK || (state == SQLITE_IOERR_SHORT_READ && ( *readSize > 0 )) ) {
        //There is content, so merge for the read size
        //The original read is correct, but possibly a short read
        mergeCiphertext((encryptFile*)file, buffer, original, offest, (int)bufferSize);
    }
    free(original);
    return state;
}
int _randomWrite(void *file, char *buffer, unsigned long long offest, unsigned int bufferSize) {
    updateBitmap(file, offest, bufferSize);
    return modifingFile->pMethods->xWrite(modifingFile, (void*)buffer, bufferSize, offest);
}

int _origRandomRead(void *file, char *buffer, unsigned long long offest, unsigned int bufferSize, unsigned int *readSize) {
    int state = underlyingFile->pMethods->xRead(underlyingFile, (void*)buffer, bufferSize, offest);
    if (state == SQLITE_IOERR_SHORT_READ) {
        //Short read, so get the read size
        sqlite_int64 size;
        ((sqlite3_file*)file)->pMethods->xFileSize(file, &size);
        *readSize = (unsigned int)(size-offest);
    } else *readSize = bufferSize;
    return state;
}
int _origRandomWrite(void *file, const char *buffer, unsigned long long offest, unsigned int bufferSize) {
    return underlyingFile->pMethods->xWrite(underlyingFile, (void*)buffer, bufferSize, offest);
}

//IO Function
int encryptedClose(sqlite3_file *file) {
    if ( ((encryptFile*)file)->passThr ) {
        int res = underlyingFile->pMethods->xClose(underlyingFile);;
        free(underlyingFile);
        return res;
    }
    int state = underlyingFile->pMethods->xClose(underlyingFile);
    state |= modifingFile->pMethods->xClose(modifingFile);
    closeBitmap(file);
    fclose(((encryptFile*)file)->bitMapFile);
    
    //Free memory
    free(underlyingFile);
    free(modifingFile);
    free(((encryptFile*)file)->key);
    
    //Merging
    encryptSqlite3_Merger(((encryptFile*) file)->addr);
    
    assert(state == 0);
    return state;
}
int encryptedRead(sqlite3_file *file, void* content, int iAmt, sqlite3_int64 iOfst) {
    if ( ((encryptFile*)file)->passThr ) return underlyingFile->pMethods->xRead(underlyingFile, content, iAmt, iOfst);
    sqlite3_int64 size;
    file->pMethods->xFileSize(file, &size);
    int state = deltaDecrypt(file, ((encryptFile*)file)->key, ((encryptFile*)file)->keySize, (void*)content, iOfst, iAmt, _origRandomRead, _randomRead);
    return state;
}
int growDatabase(encryptFile *file, sqlite_int64 lastOffset, int lastLen) {
    //Before the growth, the cached file size is correct
    sqlite_int64 size = file->actualFileSize;
    sqlite3_file *actualF = file->actualFile;
    //Read the last block
    char iv[16];
    if (size >= 16)
        actualF->pMethods->xRead(actualF, iv, 16, size-16);
    else bzero(iv, 16);
    //Generate emptiness
    char *empty = malloc(sizeof(char)*(lastOffset+lastLen-size));
    bzero(empty, (lastOffset+lastLen-size) );
    int state = randomEncrypt(file, file->key, file->keySize, empty, size, (lastOffset+lastLen-size), true, _origRandomRead, _randomRead, _origRandomWrite);
    free(empty);
    //After the growth, the size is incorrect, so upgrade it
    sqlite_int64 newSize;
    actualF->pMethods->xFileSize(actualF, &newSize);
    //But before of that, grow the database first
    growBitmap(file, newSize);
    ((encryptFile*)file)->actualFileSize = newSize;
    return state;
}
int encryptedWrite(sqlite3_file *file, const void*content, int iAmt, sqlite3_int64 iOfst) {
    if ( ((encryptFile*)file)->passThr ) {
        int d = underlyingFile->pMethods->xWrite(underlyingFile, content, iAmt, iOfst);
        return d;
    }
    sqlite3_int64 size;
    file->pMethods->xFileSize(file, &size);
    sqlite_int64 ivEnd = iOfst+iAmt;
    //Grow the file
    if (size < ivEnd) {
        growDatabase((encryptFile*)file, iOfst, iAmt);
    }
    return deltaEncrypt(file, ((encryptFile*)file)->key, ((encryptFile*)file)->keySize, (void*)content, iOfst, iAmt, size <= iAmt+iOfst, _origRandomRead, _randomRead, _randomWrite);
}
int encryptedTruncate(sqlite3_file *file, sqlite3_int64 size) {
    return underlyingFile->pMethods->xTruncate(underlyingFile, size);
}
int encryptedSync(sqlite3_file *file, int flags) {
    if ( ((encryptFile*)file)->passThr ) return underlyingFile->pMethods->xSync(underlyingFile, flags);
    //Before the sync, flush the bitmap first
    saveBitmap(file);
    int state = underlyingFile->pMethods->xSync(underlyingFile, flags);
    assert(state == 0);
    return state;
}
int encryptedFileSize(sqlite3_file *file, sqlite3_int64 *pSize) {
    return underlyingFile->pMethods->xFileSize(underlyingFile, pSize);
}
int encryptedLock(sqlite3_file *file, int flag) {
    return underlyingFile->pMethods->xLock(underlyingFile, flag);
}
int encryptedUnlock(sqlite3_file *file, int flag) {
    return underlyingFile->pMethods->xUnlock(underlyingFile, flag);
}
int encryptedCheckReservedLock(sqlite3_file *file, int *pResOut) {
    return underlyingFile->pMethods->xCheckReservedLock(underlyingFile, pResOut);
}
int encryptedFileControl(sqlite3_file *file, int op, void *pArg) {
    return underlyingFile->pMethods->xFileControl(underlyingFile, op, pArg);
}
int encryptedSectorSize(sqlite3_file *file) {
    return underlyingFile->pMethods->xSectorSize(underlyingFile);
}
int encryptedDeviceCharacteristics(sqlite3_file *file) {
    return underlyingFile->pMethods->xDeviceCharacteristics(underlyingFile);
}
/* Methods above are valid for version 1 */
int encryptedShmMap(sqlite3_file *file, int iPg, int pgsz, int flag, void volatile** ctn) {
    return underlyingFile->pMethods->xShmMap(underlyingFile, iPg, pgsz, flag, ctn);
}
int encryptedShmLock(sqlite3_file *file, int offset, int n, int flags) {
    int state = underlyingFile->pMethods->xShmLock(underlyingFile, offset, n, flags);
    assert(state == 0);
    return state;
}
void encryptedShmBarrier(sqlite3_file *file) { underlyingFile->pMethods->xShmBarrier(underlyingFile); }
int encryptedShmUnmap(sqlite3_file *file, int deleteFlag) {
    int state = underlyingFile->pMethods->xShmUnmap(underlyingFile, deleteFlag);
    assert(state == 0);
    return state;
}
/* Methods above are valid for version 2 */
int encryptedFetch(sqlite3_file *file, sqlite3_int64 iOfst, int iAmt, void **pp) {
    int state = underlyingFile->pMethods->xFetch(underlyingFile, iOfst, iAmt, pp);
    assert(state == 0);
    return state;
}
int encryptedUnfetch(sqlite3_file *file, sqlite3_int64 iOfst, void *p) {
    int state = underlyingFile->pMethods->xUnfetch(underlyingFile, iOfst, p);
    assert(state == 0);
    return state;
}


sqlite3_io_methods encryptedMethod = {
    3,
    encryptedClose,                      /* xClose */                                  \
    encryptedRead,                   /* xRead */                                   \
    encryptedWrite,                  /* xWrite */                                  \
    encryptedTruncate,               /* xTruncate */                               \
    encryptedSync,                   /* xSync */                                   \
    encryptedFileSize,               /* xFileSize */                               \
    encryptedLock,                       /* xLock */                                   \
    encryptedUnlock,                     /* xUnlock */                                 \
    encryptedCheckReservedLock,                     /* xCheckReservedLock */                      \
    encryptedFileControl,            /* xFileControl */                            \
    encryptedSectorSize,             /* xSectorSize */                             \
    encryptedDeviceCharacteristics,  /* xDeviceCapabilities */                     \
    encryptedShmMap,                     /* xShmMap */                                 \
    encryptedShmLock,                /* xShmLock */                                \
    encryptedShmBarrier,             /* xShmBarrier */                             \
    encryptedShmUnmap,               /* xShmUnmap */                               \
    encryptedFetch,                  /* xFetch */                                  \
    encryptedUnfetch,                /* xUnfetch */
};


encrypedVFS *encryptionVFS(sqlite3_vfs *originalVFS) {
    
    //First, retain the original VFS in the
    encrypedVFS *cloneVFS = malloc(sizeof(encrypedVFS));
    
    //Then, preserve original VFS
    cloneVFS->underneathVFS = originalVFS;
    
    //Configurate new VFS
    cloneVFS->superclass.pAppData = originalVFS->pAppData;
    cloneVFS->superclass.mxPathname = originalVFS->mxPathname;
    cloneVFS->superclass.szOsFile = sizeof(encryptFile);
    unsigned long nameLen = (strlen(originalVFS->zName) + 1 + 5);
    cloneVFS->superclass.zName = malloc(sizeof(char) * nameLen );
    snprintf((char*)cloneVFS->superclass.zName, nameLen, "Encry%s", originalVFS->zName);
    
    //Overrider functions
    replicateVFSFuncPtr(Open)
    replicateVFSFuncPtr(Delete)
    replicateVFSFuncPtr(Access)
    replicateVFSFuncPtr(FullPathname)
    replicateVFSFuncPtr(DlOpen)
    replicateVFSFuncPtr(DlError)
    replicateVFSFuncPtr(DlSym)
    replicateVFSFuncPtr(DlClose)
    replicateVFSFuncPtr(Randomness)
    replicateVFSFuncPtr(Sleep)
    replicateVFSFuncPtr(CurrentTime)
    replicateVFSFuncPtr(GetLastError)
    replicateVFSFuncPtr(CurrentTimeInt64)
    replicateVFSFuncPtr(SetSystemCall)
    replicateVFSFuncPtr(GetSystemCall)
    replicateVFSFuncPtr(NextSystemCall)
    
    return cloneVFS;
}

void encryptionSqliteInit() {
    sqlite3_vfs *ptr = sqlite3_vfs_find(NULL);
    while (ptr != NULL) {
        sqlite3_vfs *encryPtr = (sqlite3_vfs *)encryptionVFS(ptr);
        sqlite3_vfs_register(encryPtr, 0);
        ptr = ptr->pNext;
    }
}

sqlite3_vfs *fetchVFS(const char *originalName) {
    sqlite3_vfs *old = sqlite3_vfs_find(originalName);
    if (old == NULL) return NULL;
    char buffer[64];
    snprintf(buffer, 64, "Encry%s", old->zName);
    return sqlite3_vfs_find(buffer);
}

int encryptSqlite3_open(const char *filename, sqlite3 **ppDb) {
    return encryptSqlite3_open_v2(filename, ppDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
}

int encryptSqlite3_open_v2(const char *filename, sqlite3 **ppDb, int flags, const char *zVfs) {
    sqlite3_vfs *vfs = fetchVFS(zVfs);
    if (vfs) {
        int state = sqlite3_open_v2(filename, ppDb, flags, vfs->zName);
        char *err;
        char *sql = "PRAGMA page_size = 4096;";
        state |= sqlite3_exec(*ppDb, sql, NULL, 0, &err);
        sql = "PRAGMA journal_mode=WAL;";
        state |= sqlite3_exec(*ppDb, sql, NULL, 0, &err);
        return state;
    } else return SQLITE_FAIL;
}


void encryptSqlite3_Merger(const char *addr) {
    //Get file size
    struct stat st;
    stat(addr, &st);
    unsigned long long size = st.st_size;
    
    //Open original file
    size_t addrLen = strlen(addr);
    int dbFile = open(addr, O_RDONLY);
    char *oldBuf = (char*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, dbFile, 0);
    
    //Open modified file
    char *resultAddr = malloc(sizeof(char)*(addrLen+7));
    bcopy(addr, resultAddr, addrLen);
    bcopy(".delta", &(resultAddr[addrLen]), 7);
    int deltaFile = open(resultAddr, O_RDONLY);
    char *deltaBuf = (char*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, deltaFile, 0);
    
    bcopy(".map", &(resultAddr[addrLen]), 5);
    int mapFile = open(resultAddr, O_RDONLY);
    bool *mapBuf = (bool*)mmap(NULL, size/4, PROT_READ, MAP_PRIVATE, mapFile, 0);
    
    //Open output
    bcopy(".store", &(resultAddr[addrLen]), 7);
    int storeFile = open(resultAddr, O_RDWR | O_CREAT | O_NONBLOCK | O_TRUNC, 420);
    ftruncate(storeFile, size);
    char *storeBuf = (char*)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, storeFile, 0);
    
    //Key
    char key[32];
    copyMasterKey_self(addr, key);
    
    char allZero[16];
    bzero(allZero, 16);
    
    for (unsigned long long i = 0; i < size; i+=16) {
        unsigned long long iDiv16 = i >> 4;
        //For every block
        //Read iv
        char *decryIV = i == 0? allZero: &oldBuf[i-16];
        char *encryIV = i == 0? allZero: &storeBuf[i-16];
        //Read content
        char *inputPtr = mapBuf[iDiv16]? &(deltaBuf[i]): &(oldBuf[i]);
        char plaintext[16];
        lowLevelMaskingDecrypt(key, 32, decryIV, inputPtr, 16, plaintext);
        lowLevelEncrypt(key, 32, encryIV, 16, plaintext, 16, &(storeBuf[i]));
    }
    
    munmap(storeBuf, size);
    
    close(storeFile);
    
    //Finish merging
    unlink(addr);
    bcopy(".delta", &(resultAddr[addrLen]), 7);
    unlink(resultAddr);
    bcopy(".map", &(resultAddr[addrLen]), 5);
    unlink(resultAddr);
    
    bcopy(".store", &(resultAddr[addrLen]), 7);
    rename(resultAddr, addr);
}
