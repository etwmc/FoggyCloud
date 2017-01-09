//
//  main.c
//  databaseMerger
//
//  Created by Wai Man Chan on 1/8/17.
//
//

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/mman.h>

#include "CVtKeyManagment.h"
#include "EncryptionBlocker.h"

int symmetryKeySize = 32;
int asymmetryKeySize = 512;

int main(int argc, const char * argv[]) {
    //File address
    const char *addr = argv[1];
    
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
    
    return 0;
}
