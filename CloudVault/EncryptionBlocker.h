//
//  EncryptionBlocker.h
//  CloudVault
//
//  Created by Wai Man Chan on 12/1/16.
//
//

#ifndef EncryptionBlocker_h
#define EncryptionBlocker_h

#include <stdio.h>

//AES
int lowLevelEncrypt(const char *key, unsigned int keySize, const char *iv, unsigned int ivSize, const char *input, unsigned int inputSize, char *output);
int lowLevelDecrypt(const char *key, unsigned int keySize, const char *iv, unsigned int ivSize, const char *input, unsigned int inputSize, char *output);
int lowLevelMaskingEncrypt(const char *key, unsigned int keySize, const char *mask, const char *input, unsigned int inputSize, char *output);
int lowLevelMaskingDecrypt(const char *key, unsigned int keySize, const char *mask, const char *input, unsigned int inputSize, char *output);
void generateSymmetricKey(const char *keyBuf, size_t size);

//RSA
void generateRSAKey(const char *privKey, const char *pubKey, size_t size, size_t *privKeySize, size_t *pubKeySize);
#endif /* EncryptionBlocker_h */
