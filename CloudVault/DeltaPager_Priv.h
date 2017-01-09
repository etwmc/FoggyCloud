//
//  DeltaPager_Priv.h
//  CloudVault
//
//  Created by Wai Man Chan on 1/8/17.
//
//

#ifndef DeltaPager_Priv_h
#define DeltaPager_Priv_h

#include <sqlite3.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    sqlite3_io_methods *method;
    sqlite3_vfs *pVfs;                  /* The VFS that created this encryptFile */
    sqlite3_file *actualFile;
    sqlite3_file *modifyFile;
    sqlite3_int64 actualFileSize;
    //Modified bits
    FILE *bitMapFile;
    bool *modifiedBit;
    //Encryption
    const char *key;
    unsigned int keySize;
} encryptFile;

typedef struct {
    sqlite3_vfs superclass;
    sqlite3_vfs *underneathVFS;
    sqlite3_io_methods fileMethod;
    int pageSize;
} encrypedVFS;

//Mapping service
void loadBitmap(encryptFile *file);

void saveBitmap(encryptFile *file);

void growBitmap(encryptFile *file, sqlite_int64 newSize);

void updateBitmap(encryptFile *file, sqlite_int64 offset, int amount);

//Put the original ciphertext (unmodified) back to the ciphertext(readed ciphertext)
void mergeCiphertext(encryptFile *file, char *ciphertext, const char *original, sqlite_int64 offset, int length);

int encryptOpen(sqlite3_vfs* vfs, const char *zName, sqlite3_file* file, int flags, int *pOutFlags);
int encryptDelete(sqlite3_vfs* vfs, const char *zName, int syncDir);
int encryptAccess(sqlite3_vfs* vfs, const char *zName, int flags, int *pResOut);
int encryptFullPathname(sqlite3_vfs* vfs, const char *zName, int nOut, char *zOut);
void *encryptDlOpen(sqlite3_vfs* vfs, const char *zFilename);
void encryptDlError(sqlite3_vfs* vfs, int nByte, char *zErrMsg);
void (*encryptDlSym(sqlite3_vfs *vfs, void *p, const char*zSym))(void);
void encryptDlClose(sqlite3_vfs* vfs, void*ptr);
int encryptRandomness(sqlite3_vfs* vfs, int nByte, char *zOut);
int encryptSleep(sqlite3_vfs* vfs, int microseconds);
int encryptCurrentTime(sqlite3_vfs* vfs, double*ptr);
int encryptGetLastError(sqlite3_vfs* vfs, int size, char *buffer);
int encryptCurrentTimeInt64(sqlite3_vfs* vfs, sqlite3_int64*ptr);
int encryptSetSystemCall(sqlite3_vfs* vfs, const char *zName, sqlite3_syscall_ptr ptr);
sqlite3_syscall_ptr encryptGetSystemCall(sqlite3_vfs* vfs, const char *zName);
const char *encryptNextSystemCall(sqlite3_vfs* vfs, const char *zName);

#endif /* DeltaPager_Priv_h */
