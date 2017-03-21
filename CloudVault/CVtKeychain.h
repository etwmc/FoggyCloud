//
//  CVtKeychain.h
//  CloudVault
//
//  Created by Wai Man Chan on 3/4/17.
//
//

#ifndef CVtKeychain_h
#define CVtKeychain_h

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    char UUID[37];
} userID;

userID CVt_selfUserID();
bool CVt_userIsSelf(userID user);

size_t CVt_getPublicKey(userID user, char **publicKey);
size_t CVt_getPrivateKey(userID user, char **privateKey);

void CVt_keychainBootup();

/*
 * Memory Protection
 * Before free a memory space, the space is wiped by zero in order to protect the key from buffer overflow
 */
void securelyFree(void *ptr, size_t size);

#endif /* CVtKeychain_h */
