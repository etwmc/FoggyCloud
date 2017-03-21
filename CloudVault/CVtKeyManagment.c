//
//  CVtKeyManagment.c
//  CloudVault
//
//  Created by Wai Man Chan on 12/28/16.
//
//

#include "CVtKeyManagment.h"
#include <stdlib.h>
#include <stdio.h>
#include "EncryptionBlocker.h"
#include <sys/stat.h>

extern int symmetryKeySize;
extern int asymmetryKeySize;

bool keybagExist(const char* addr) {
    char*keybag = (char*)malloc(sizeof(char)*4096);
    snprintf(keybag, 4096, "%s.keybag", addr);
    
    struct stat stat_buf;
    int rc = stat(keybag, &stat_buf);
    
    free(keybag);
    
    int fileSize = rc == 0 ? stat_buf.st_size : -1;
    
    return fileSize > 0;
}

size_t copyMasterKey_self(const char *addr, const char *buffer) {
    char*keybag = (char*)malloc(sizeof(char)*4096);
    snprintf(keybag, 4096, "%s.keybag", addr);
    FILE *f = fopen(keybag, "r");
    free(keybag);
    fseek(f, 0, SEEK_SET);
    size_t ret = fread((void*)buffer, sizeof(char), symmetryKeySize, f);
    fclose(f);
    return ret;
}

size_t copyPrivateKey_self(const char *addr, const char *buffer) {
    //Unknown length, so copy everything
    char*keybag = (char*)malloc(sizeof(char)*4096);
    snprintf(keybag, 4096, "%s.keybag", addr);
    FILE *f = fopen(keybag, "r");
    free(keybag);
    fseek(f, symmetryKeySize, SEEK_SET);
    size_t ret = fread((void*)buffer, sizeof(char), 4096, f);
    fclose(f);
    return ret;
}

size_t copyPublicKey_self(const char *addr, const char *buffer) {
    char*keybag = (char*)malloc(sizeof(char)*4096);
    snprintf(keybag, 4096, "%s.keybag", addr);
    FILE *f = fopen(keybag, "r");
    free(keybag);
    fseek(f, symmetryKeySize+asymmetryKeySize, SEEK_SET);
    size_t ret = fread((void*)buffer, sizeof(char), asymmetryKeySize, f);
    fclose(f);
    return ret;
}

void generateKeyBag(const char* addr) {
    char*keybag = (char*)malloc(sizeof(char)*4096);
    snprintf(keybag, 4096, "%s.keybag", addr);
    FILE *f = fopen(keybag, "w");
    snprintf(keybag, 4096, "%s.keybag.pub", addr);
    FILE *pubF = fopen(keybag, "w");
    free(keybag);
    
    char *master = malloc(sizeof(char)*symmetryKeySize);
    generateSymmetricKey(master, symmetryKeySize);
    fwrite(master, sizeof(char), symmetryKeySize, f);
    free(master);
    
    size_t privateKeySize, publicKeySize;
    char *priv = malloc(sizeof(char)*4096);
    char *pub = malloc(sizeof(char)*4096);
    generateRSAKey(priv, pub, asymmetryKeySize, &privateKeySize, &publicKeySize);
    fwrite(pub, sizeof(char), publicKeySize, pubF);
    fwrite(priv, sizeof(char), privateKeySize, f);
    free(priv);
    free(pub);
    
    fclose(pubF);
    fclose(f);
}
