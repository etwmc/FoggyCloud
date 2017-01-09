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


//VFS functions
#define underlyingVFS ((encrypedVFS*)vfs)->underneathVFS
#define underlyingFile ((encryptFile*)file)->actualFile
#define modifingFile ((encryptFile*)file)->modifyFile

extern sqlite3_io_methods encryptedMethod;

//Mapping service
void loadBitmap(encryptFile *file) {
    //Get actual size, so map
    //Because of page schema, the file size will be an integer of the (blockSize * 8)
    
    fseek(file->bitMapFile, 0, SEEK_SET);
    size_t ret = fread(file->modifiedBit, sizeof(bool), file->actualFileSize/16, file->bitMapFile);
    if (ret != file->actualFileSize) {
        //It means during the last edit, we growed the original text after the
        for (int i = ret; i < file->actualFileSize/16; i++)
            file->modifiedBit[i] = false;
    }
}

void saveBitmap(encryptFile *file) {
    //Get actual size, so map
    //Because of page schema, the file size will be an integer of the (blockSize * 8)
    fseek(file->bitMapFile, 0, SEEK_SET);
    fwrite(file->modifiedBit, sizeof(bool), file->actualFileSize/16, file->bitMapFile);
    fflush(file->bitMapFile);
}

void growBitmap(encryptFile *file, sqlite_int64 newSize) {
    bool *newVector = malloc(sizeof(bool)*newSize/16);
    bcopy(file->modifiedBit, newVector, file->actualFileSize/16);
    for (sqlite_int64 j = file->actualFileSize/16; j < newSize/16; j++) {
        newVector[j] = false;
    }
    free(file->modifiedBit);
    file->modifiedBit = newVector;
}

void updateBitmap(encryptFile *file, sqlite_int64 offset, int amount) {
    //Fill the section
    sqlite_int64 offsetDiv4 = offset/16;
    int amountDiv4 = amount/16;
    bool *bit = file->modifiedBit;
    for (int i = 0; i < amountDiv4; i++) {
        bit[i+offsetDiv4] = true;
    }
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
    
    //New address: zName.delta
    int newAddrLen = strlen(zName)+7;
    char *newAddr = malloc(sizeof(char)*newAddrLen);
    bcopy(zName, newAddr, strlen(zName));
    bcopy(".delta", newAddr+strlen(zName), 7);
    
    //Initial underneath vfs
    int resNo = underlyingVFS->xOpen(underlyingVFS, zName, rootFile, flags, pOutFlags);
    resNo |= underlyingVFS->xOpen(underlyingVFS, newAddr, deltaFile, flags, pOutFlags);
    if (resNo == SQLITE_OK) {
        //Sucess, so continuous initial
        ((encryptFile*)file)->actualFile = rootFile;
        rootFile->pMethods->xFileSize(rootFile, &((encryptFile*)file)->actualFileSize);
        
        //Get actual size, so map
        ((encryptFile*)file)->modifiedBit = malloc(sizeof(bool)*((encryptFile*)file)->actualFileSize/4);
        bzero(((encryptFile*)file)->modifiedBit, ((encryptFile*)file)->actualFileSize/4);
        bcopy(".map", newAddr+strlen(zName), 5);
        FILE *f = fopen(newAddr, "r");
        if (f == NULL) {
            //Init file
            f = fopen(newAddr, "w");
            fclose(f);
            f = fopen(newAddr, "r");
        }
        ((encryptFile*)file)->bitMapFile = f;
        loadBitmap(file);
        f = fopen(newAddr, "w+");
        ((encryptFile*)file)->bitMapFile = f;
        saveBitmap(file);
        
        ((encryptFile*)file)->modifyFile = deltaFile;
        ((encryptFile*)file)->method = &encryptedMethod;
        ((encryptFile*)file)->pVfs = vfs;
        
        //Init key
        ((encryptFile*)file)->keySize = 32;
        ((encryptFile*)file)->key = malloc(sizeof(char)*32);
        //Check key bag
        if (!keybagExist(zName))
            generateKeyBag(zName);
        copyMasterKey_self(zName, ((encryptFile*)file)->key);
        
    }
    free(newAddr);
    return resNo;
}
int encryptDelete(sqlite3_vfs* vfs, const char *zName, int syncDir) {
    //Delete file
    int state = underlyingVFS->xDelete(underlyingVFS, zName, syncDir);
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
int encryptAccess(sqlite3_vfs* vfs, const char *zName, int flags, int *pResOut) {
    int state = underlyingVFS->xAccess(underlyingVFS, zName, flags, pResOut);
    return state;
}
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
    int state = underlyingFile->pMethods->xClose(underlyingFile);
    state |= modifingFile->pMethods->xClose(modifingFile);
    fclose(((encryptFile*)file)->bitMapFile);
    
    //Free memory
    free(underlyingFile);
    free(modifingFile);
    free(((encryptFile*)file)->modifiedBit);
    free(((encryptFile*)file)->key);
    
    assert(state == 0);
    return state;
}
int encryptedRead(sqlite3_file *file, void* content, int iAmt, sqlite3_int64 iOfst) {
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
    int state = underlyingFile->pMethods->xTruncate(underlyingFile, size);
    assert(state == 0);
    return state;
}
int encryptedSync(sqlite3_file *file, int flags) {
    //Before the sync, flush the bitmap first
    saveBitmap(file);
    int state = underlyingFile->pMethods->xSync(underlyingFile, flags);
    assert(state == 0);
    return state;
}
int encryptedFileSize(sqlite3_file *file, sqlite3_int64 *pSize) {
    int state = underlyingFile->pMethods->xFileSize(underlyingFile, pSize);
    assert(state == 0);
    return state;
}
int encryptedLock(sqlite3_file *file, int flag) {
    int state = underlyingFile->pMethods->xLock(underlyingFile, flag);
    assert(state == 0);
    return state;
}
int encryptedUnlock(sqlite3_file *file, int flag) {
    int state = underlyingFile->pMethods->xUnlock(underlyingFile, flag);
    assert(state == 0);
    return state;
}
int encryptedCheckReservedLock(sqlite3_file *file, int *pResOut) {
    int state = underlyingFile->pMethods->xCheckReservedLock(underlyingFile, pResOut);
    assert(state == 0);
    return state;
}
int encryptedFileControl(sqlite3_file *file, int op, void *pArg) {
    int state = underlyingFile->pMethods->xFileControl(underlyingFile, op, pArg);
    
    return state;
}
int encryptedSectorSize(sqlite3_file *file) {
    int state = underlyingFile->pMethods->xSectorSize(underlyingFile);
    assert(state == 0);
    return state;
}
int encryptedDeviceCharacteristics(sqlite3_file *file) {
    int state = underlyingFile->pMethods->xDeviceCharacteristics(underlyingFile);
    return state;
}
/* Methods above are valid for version 1 */
int encryptedShmMap(sqlite3_file *file, int iPg, int pgsz, int flag, void volatile** ctn) {
    int state = underlyingFile->pMethods->xShmMap(underlyingFile, iPg, pgsz, flag, ctn);
    assert(state == 0);
    return state;
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
    char sql[] = "PRAGMA page_size = 4096;";
    sqlite3_vfs *vfs = fetchVFS(zVfs);
    if (vfs) {
        int state = sqlite3_open_v2(filename, ppDb, flags, vfs->zName);
        char *err;
        state |= sqlite3_exec(*ppDb, sql, NULL, 0, &err);
        return state;
    } else return SQLITE_FAIL;
}