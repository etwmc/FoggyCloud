//
//  CVt_FileManagement.c
//  CloudVault
//
//  Created by Wai Man Chan on 2/14/17.
//
//

#include "CVt_FileManagement.h"
#include <strings.h>
#include <stdlib.h>
#include <sys/stat.h>

void CVt_rawFlushDown(const char *buffer, size_t len, const char *addressPrefix, const char *address) {
    //Combine address
    size_t prefixLen = strlen(addressPrefix);
    size_t addrLen = strlen(address);
    char *combinedAddr = malloc(sizeof(char)*(prefixLen+addrLen+1));
    bcopy(addressPrefix, combinedAddr, prefixLen);
    bcopy(address, combinedAddr+prefixLen, addrLen);
    
    FILE *f = fopen(combinedAddr, "a+");
    free(combinedAddr);
    fwrite(buffer, len, 1, f);
    fclose(f);
}

size_t CVt_rawReadback(const char **buffer, const char *addressPrefix, const char *address) {
    //Combine address
    size_t prefixLen = strlen(addressPrefix);
    size_t addrLen = strlen(address);
    char *combinedAddr = malloc(sizeof(char)*(prefixLen+addrLen+1));
    bcopy(addressPrefix, combinedAddr, prefixLen);
    bcopy(address, combinedAddr+prefixLen, addrLen);
    
    //Get file size
    struct stat st;
    stat(combinedAddr, &st);
    unsigned long long size = st.st_size;
    
    //Allocate the buffer
    *buffer = malloc(sizeof(char)*size);
    
    FILE *f = fopen(combinedAddr, "r");
    free(combinedAddr);
    fread(*buffer, size, 1, f);
    fclose(f);
    
    return size;
}

size_t CVt_offsetReadback(const char **buffer, size_t offset, const char *addressPrefix, const char *address) {
    //Combine address
    size_t prefixLen = strlen(addressPrefix);
    size_t addrLen = strlen(address);
    char *combinedAddr = malloc(sizeof(char)*(prefixLen+addrLen+1));
    bcopy(addressPrefix, combinedAddr, prefixLen);
    bcopy(address, combinedAddr+prefixLen, addrLen);
    
    //Get file size
    struct stat st;
    stat(combinedAddr, &st);
    unsigned long long size = st.st_size;
    if (offset >= size) {
        *buffer = NULL;
        return 0;
    }
    
    size -= offset;
    
    //Allocate the buffer
    *buffer = malloc(sizeof(char)*size);
    
    FILE *f = fopen(combinedAddr, "r");
    free(combinedAddr);
    fseek(f, offset, SEEK_SET);
    fread(*buffer, size, 1, f);
    fclose(f);
    
    return size;
}

size_t CVt_partialReadback(const char **buffer, size_t size, const char *addressPrefix, const char *address) {
    //Combine address
    size_t prefixLen = strlen(addressPrefix);
    size_t addrLen = strlen(address);
    char *combinedAddr = malloc(sizeof(char)*(prefixLen+addrLen+1));
    bcopy(addressPrefix, combinedAddr, prefixLen);
    bcopy(address, combinedAddr+prefixLen, addrLen);
    
    //Allocate the buffer
    *buffer = malloc(sizeof(char)*size);
    
    FILE *f = fopen(combinedAddr, "r");
    free(combinedAddr);
    fread(*buffer, size, 1, f);
    fclose(f);
    
    return size;
}

bool CVt_FileManagmentRequestCanFulfill(const char *addressPrefix, const char *address) {
    //Combine address
    size_t prefixLen = strlen(addressPrefix);
    size_t addrLen = strlen(address);
    char *combinedAddr = malloc(sizeof(char)*(prefixLen+addrLen+1));
    bcopy(addressPrefix, combinedAddr, prefixLen);
    bcopy(address, combinedAddr+prefixLen, addrLen);
    
    static struct stat buffer;
    return (stat (address, &buffer) == 0);
}
