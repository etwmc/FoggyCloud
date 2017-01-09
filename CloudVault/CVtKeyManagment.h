//
//  CVtKeyManagment.h
//  CloudVault
//
//  Created by Wai Man Chan on 12/28/16.
//
//

#ifndef CVtKeyManagment_h
#define CVtKeyManagment_h

#include <stdbool.h>
#include <stdio.h>

//Check keybag
bool keybagExist(const char* addr);

//Raw key
size_t copyMasterKey_self(const char *addr, const char *buffer);
//PEM key
size_t copyPrivateKey_self(const char *addr, const char *buffer);
size_t copyPublicKey_self(const char *addr, const char *buffer);

void generateKeyBag(const char* addr);

#endif /* CVtKeyManagment_h */
