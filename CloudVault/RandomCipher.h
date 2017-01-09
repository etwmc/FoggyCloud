//
//  RandomCipher.hpp
//  CloudVault
//
//  Created by Wai Man Chan on 12/28/16.
//
//

#ifndef RandomCipher_hpp
#define RandomCipher_hpp

#include <stdio.h>
#include <stdbool.h>

//Used to read IV and content
typedef int(*randomRead)(void *fd, char *buffer, unsigned long long offest, unsigned int bufferSize, unsigned int *readSize);
typedef int(*randomWrite)(void *fd, const char *buffer, unsigned long long offest, unsigned int bufferSize);

int randomEncrypt(void *fd, const char *key, unsigned int keySize, char *plaintext, unsigned long long offset, unsigned int plaintextSize, bool tail, randomRead origReader, randomRead newTextReader, randomWrite writer);
int randomDecrypt(void *fd, const char *key, unsigned int keySize, char *plaintext, unsigned long long offset, unsigned int plaintextSize, randomRead origReader, randomRead newTextReader);
int deltaEncrypt(void *fd, const char *key, unsigned int keySize, char *plaintext, unsigned long long offset, unsigned int plaintextSize, bool tail, randomRead origReader, randomRead newTextReader, randomWrite writer);
int deltaDecrypt(void *fd, const char *key, unsigned int keySize, char *plaintext, unsigned long long offset, unsigned int plaintextSize, randomRead origReader, randomRead newTextReader);

#endif /* RandomCipher_hpp */
