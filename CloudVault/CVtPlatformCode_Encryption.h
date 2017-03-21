//
//  CVtPlatformCode_Encryption.h
//  CloudVault
//
//  Created by Wai Man Chan on 2/21/17.
//
//

#ifndef CVtPlatformCode_Encryption_h
#define CVtPlatformCode_Encryption_h

#include <stdio.h>
#include <stdlib.h>

/* AES 256-CBC
 * Key: 256 bit
 * iv: 128 bit
 * Return value: the actual length of the ciphertext, negative means fail
 * Padding: PKCS7
 */
size_t CVtPlatformCode_Encryption_AES256_CFB(const char *content, size_t contentLen, const char *key, const char *iv, char **output);
size_t CVtPlatformCode_Decryption_AES256_CFB(const char *ciphertext, size_t ciphertextLen, const char *key, const char *iv, char **output);

/* RSA4096
 * Private Key: RSA Public Key cert in PEM
 * Public Key: Private Key in PEM
 * Not padding, in-out are the same size
 */
int CVtPlatformCode_Encryption_RSA4096(const char *plaintext, size_t platintextLen, const char *publicKey, char *ciphertext);
int CVtPlatformCode_Decryption_RSA4096(const char *ciphertext, size_t ciphertextLen, const char *privateKey, char *output);

/* SHA 512
 * Input: plaintext, length
 * Outcome: 64B hash
 */
void CVtPlatformCode_Hash_SHA512(const char *plaintext, size_t plaintextLen, char *output);

/*
 * Security key storage
 * This is used to store the private keys, it should be store in protected area that only the program itself can access
 * The key is accessed using Key-Value style storage, although in current development only two key is stored (the user RSA4096 private key and the SQLite AES 256 key)
 * WARNING: failure to protect the content inside trusted, secured storage WILL RENDER ALL encryption used in the program USELESS
 */
void CVtPlatformCode_SafeStorage_Set(const char *keyOwner, const char *keyType, const char *content, size_t contentSize);
size_t CVtPlatformCode_SafeStorage_Get(const char *keyOwner, const char *keyType, const char **content);

#endif /* CVtPlatformCode_Encryption_h */
