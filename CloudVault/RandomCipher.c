//
//  RandomCipher.cpp
//  CloudVault
//
//  Created by Wai Man Chan on 12/28/16.
//
//

#include "RandomCipher.h"
#include "EncryptionBlocker.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include <sqlite3.h>

//AES 256 block size has 128 bit per block
const unsigned int blockSize = 16;

unsigned long long blockOffset(unsigned long long original) {
    return original-(original%blockSize);
}

unsigned int newSize(unsigned long long offset, unsigned int origianlSize) {
    unsigned long long endRange = offset + origianlSize;
    endRange += endRange%blockSize? blockSize-endRange%blockSize: 0;
    unsigned long long newBegin = blockOffset(offset);
    return endRange-newBegin;
}

void partialCopy(unsigned long long offset, unsigned int originalSize, const char *input, const char *output) {
    unsigned int offsetDelta = offset-blockOffset(offset);
    bcopy(input+offsetDelta, output, originalSize);
}

inline int readPartialPage(unsigned long long offset, unsigned int size, const char *page, randomRead reader, void*fd) {
    unsigned long long pageOffset = blockOffset(offset);
    unsigned int pageSize = newSize(offset, size);
    char *tmpPage = malloc(sizeof(char)*pageSize);
    //Get reader to read
    unsigned int readSize;
    int state = reader(fd, tmpPage, pageOffset, pageSize, &readSize);
    //Copy
    partialCopy(offset, size, tmpPage, page);
    free(tmpPage);
    return state;
}

inline int writePartialPage(unsigned long long offset, unsigned int size, const char *page, randomRead reader, randomWrite writer, void *fd) {
    unsigned long long pageOffset = blockOffset(offset);
    unsigned int pageSize = newSize(offset, size);
    //unsigned long
    return SQLITE_FAIL;
}

int randomEncrypt(void *fd, const char *key, unsigned int keySize, char *plaintext, unsigned long long offset, unsigned int plaintextSize, bool tail, randomRead origReader, randomRead newTextReader, randomWrite writer) {
    
    unsigned long long pageOffset = blockOffset(offset);
    unsigned int pageSize = newSize(offset, plaintextSize);
    char *tmpPage = malloc(sizeof(char)*pageSize);
    
    //Read the contents
    if (plaintextSize != pageSize) {
        //randomDecrypt(fd, key, keySize, tmpPage, pageOffset, pageSize, origReader, newTextReader);
        /**/
         
         //Pad front
         randomDecrypt(fd, key, keySize, tmpPage, pageOffset, offset-pageOffset, origReader, newTextReader);
         //Pad back
         randomDecrypt(fd, key, keySize, tmpPage+(offset-pageOffset)+plaintextSize, offset+plaintextSize, (pageSize-plaintextSize)-(offset-pageOffset), origReader, newTextReader);
         
         /**/
    }
    
    //Replace the content
    bcopy(plaintext, tmpPage+(offset-pageOffset), plaintextSize);
    
    //Read in IV
    char ivBuffer[blockSize];
    if (offset >= blockSize) {
        //Use the cipher text as IV
        unsigned int readIVSize;
        if (origReader(fd, ivBuffer, offset-blockSize, blockSize, &readIVSize) != SQLITE_OK) {
            //Failed to read IV, complaint
            return SQLITE_FAIL;
        }
    } else {
        bzero((void*)ivBuffer, blockSize);
    }
    
    //Encrypt the page
    char *ciphertext = malloc(sizeof(char)*pageSize);
    lowLevelEncrypt(key, keySize, ivBuffer, blockSize, tmpPage, pageSize, ciphertext);
    int state = writer(fd, ciphertext, pageOffset, pageSize);
    free(tmpPage);
    free(ciphertext);
    return state;
}

int randomDecrypt(void *fd, const char *key, unsigned int keySize, char *plaintext, unsigned long long offset, unsigned int plaintextSize, randomRead origReader, randomRead newTextReader) {
    
    unsigned long long pageOffset = blockOffset(offset);
    unsigned int pageSize = newSize(offset, plaintextSize);
    char *tmpPage = malloc(sizeof(char)*pageSize);
    //Get reader to read
    unsigned int readSize;
    int state = newTextReader(fd, tmpPage, pageOffset, pageSize, &readSize);
    
    //Read in IV
    char ivBuffer[blockSize];
    if (offset >= blockSize) {
        //Use the cipher text as IV
        unsigned int readIVSize;
        if (origReader(fd, ivBuffer, offset-blockSize, blockSize, &readIVSize) != SQLITE_OK) {
            //Failed to read IV, complaint
            return SQLITE_FAIL;
        }
    } else {
        bzero((void*)ivBuffer, blockSize);
    }
    
    char *decryTmp = malloc(sizeof(char)*pageSize);
    if (readSize > 0) errno = lowLevelDecrypt(key, keySize, ivBuffer, blockSize, tmpPage, readSize, decryTmp);
    else errno = 0;
    if (errno != 0) return SQLITE_FAIL;
    //Copy
    partialCopy(offset, plaintextSize, decryTmp, plaintext);
    free((void*)decryTmp);
    free(tmpPage);
    return state;
    
}

int deltaEncrypt(void *fd, const char *key, unsigned int keySize, char *plaintext, unsigned long long offset, unsigned int plaintextSize, bool tail, randomRead origReader, randomRead newTextReader, randomWrite writer) {
    
    unsigned long long pageOffset = blockOffset(offset);
    unsigned int pageSize = newSize(offset, plaintextSize);
    char *tmpPage = malloc(sizeof(char)*pageSize);
    unsigned int readSize;
    
    //Read the contents
    if (plaintextSize != pageSize) {
        //randomDecrypt(fd, key, keySize, tmpPage, pageOffset, pageSize, origReader, newTextReader);
        /**/
        
        //Pad front
        randomDecrypt(fd, key, keySize, tmpPage, pageOffset, offset-pageOffset, origReader, newTextReader);
        //Pad back
        randomDecrypt(fd, key, keySize, tmpPage+(offset-pageOffset)+plaintextSize, offset+plaintextSize, (pageSize-plaintextSize)-(offset-pageOffset), origReader, newTextReader);
        
        /**/
    }
    
    //Replace the content
    bcopy(plaintext, tmpPage+(offset-pageOffset), plaintextSize);
    
    //Read in IV
    char *ivPage = malloc(sizeof(char)*pageSize);
    int ivHead = pageOffset-blockSize;
    int ivState = origReader(fd, ivHead>=0? ivPage: &(ivPage[blockSize]), ivHead>=0? ivHead: 0, ivHead>=0? pageSize: pageSize-16, &readSize);
    if (ivHead < 0) {
        bzero(ivPage, 16);
    }
    
    //Encrypt the page
    char *ciphertext = malloc(sizeof(char)*pageSize);
    lowLevelMaskingEncrypt(key, keySize, ivPage, tmpPage, pageSize, ciphertext);
    int state = writer(fd, ciphertext, pageOffset, pageSize);
    free(tmpPage);
    free(ciphertext);
    free(ivPage);
    return state;
}

int deltaDecrypt(void *fd, const char *key, unsigned int keySize, char *plaintext, unsigned long long offset, unsigned int plaintextSize, randomRead origReader, randomRead newTextReader) {
    
    unsigned long long pageOffset = blockOffset(offset);
    unsigned int pageSize = newSize(offset, plaintextSize);
    char *tmpPage = malloc(sizeof(char)*pageSize);
    //Get reader to read
    unsigned int readSize;
    int state = newTextReader(fd, tmpPage, pageOffset, pageSize, &readSize);
    
    //Read in IV
    char *ivPage = malloc(sizeof(char)*pageSize);
    int ivHead = pageOffset-blockSize;
    int ivState = origReader(fd, ivHead>=0? ivPage: &(ivPage[blockSize]), ivHead>=0? ivHead: 0, ivHead>=0? pageSize: pageSize-16, &readSize);
    if (ivHead < 0) {
        bzero(ivPage, 16);
    }
    
    char *decryTmp = malloc(sizeof(char)*pageSize);
    if (readSize > 0) errno = lowLevelMaskingDecrypt(key, keySize, ivPage, tmpPage, pageSize, decryTmp);
    else errno = 0;
    if (errno != 0) return SQLITE_FAIL;
    
    //Copy
    partialCopy(offset, plaintextSize, decryTmp, plaintext);
    free((void*)decryTmp);
    free(tmpPage);
    free(ivPage);
    return state;
    
}
