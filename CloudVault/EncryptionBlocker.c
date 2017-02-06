//
//  EncryptionBlocker.c
//  CloudVault
//
//  Created by Wai Man Chan on 12/1/16.
//
//

#include "EncryptionBlocker.h"
#if __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <CommonCrypto/CommonCrypto.h>
#include <Security/Security.h>
#endif

#if __APPLE__
#define tranToCFData(varName)  CFDataRef varName##Data = CFDataCreateWithBytesNoCopy(NULL, varName, varName##Size, kCFAllocatorNull);

SecKeyRef keyFromChar(const char *key, unsigned int keySize) {
    
    static SecKeyRef keyCache = NULL;
    static const char *keyPtr = NULL;
    if (keyPtr == key) return keyCache;
    else if (keyCache != NULL) {
        CFRelease(keyCache);
    }
    
    //Conf Dict
    CFStringRef keys[] = {kSecAttrKeyType, kSecAttrKeySizeInBits};
    int keyLenRaw = keySize*sizeof(char)*8;
    
    CFNumberRef keyLen = CFNumberCreate(NULL, kCFNumberSInt32Type, &keyLenRaw);
    const void* values[] = { kSecAttrKeyTypeAES, keyLen };
    CFDictionaryRef encDict = CFDictionaryCreate(NULL, keys, values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    //Key array to CFData
    tranToCFData(key)
    
    CFErrorRef error = NULL;
    SecKeyRef keyRef = SecKeyCreateFromData(encDict, keyData, &error);
    
    assert(error == NULL);
    
    CFRelease(keyData);
    
    CFRelease(encDict);
    CFRelease(keyLen);
    
    keyCache = keyRef;
    keyPtr = key;
    
    return keyRef;
}
CFDataRef convertSecKeyToData(SecKeyRef key, SecExternalFormat externalFormat) {
    CFMutableArrayRef keyUsage = CFArrayCreateMutable(
                                                      kCFAllocatorDefault,
                                                      0,
                                                      &kCFTypeArrayCallBacks
                                                      );
    
    /* This example sets a lot of usage values.
     Choose usage values that are appropriate
     to your specific task. Possible values begin
     with kSecAttrCan, and are defined in
     SecItem.h */
    CFArrayAppendValue(keyUsage, kSecAttrCanEncrypt);
    CFArrayAppendValue(keyUsage, kSecAttrCanDecrypt);
    CFArrayAppendValue(keyUsage, kSecAttrCanDerive);
    CFArrayAppendValue(keyUsage, kSecAttrCanSign);
    CFArrayAppendValue(keyUsage, kSecAttrCanVerify);
    CFArrayAppendValue(keyUsage, kSecAttrCanWrap);
    CFArrayAppendValue(keyUsage, kSecAttrCanUnwrap);
    CFMutableArrayRef keyAttributes = CFArrayCreateMutable(
                                                           kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks
                                                           );
    SecItemImportExportKeyParameters params;
    params.version = SEC_KEY_IMPORT_EXPORT_PARAMS_VERSION;
    params.flags = 0;
    params.passphrase = NULL;
    params.alertTitle = NULL;
    params.alertPrompt = NULL;
    params.accessRef = NULL;
    params.keyUsage = keyUsage;
    params.keyAttributes = keyAttributes;
    
    int flags = 0;
    CFDataRef pkdata;
    OSStatus oserr = SecItemExport(key,
                                   externalFormat, // See SecExternalFormat for details
                                   flags, // See SecItemImportExportFlags for details
                                   &params,
                                   (CFDataRef *)&pkdata);
    CFRelease(keyUsage);
    CFRelease(keyAttributes);
    return pkdata;
}

#endif

int lowLevelEncrypt(const char *key, unsigned int keySize, const char *iv, unsigned int ivSize, const char *input, unsigned int inputSize, char *output) {
    //Make sure the data is block size
    assert(keySize % 16 == 0);
#if __APPLE__
    SecKeyRef keyRef = keyFromChar(key, keySize);
    CCCryptorStatus status;
    size_t decryptedSize;
    status = CCCrypt(kCCEncrypt, kCCAlgorithmAES, 0, key, keySize, iv, input, inputSize, output, inputSize, &decryptedSize);
    assert(status == kCCSuccess);
    assert(decryptedSize == inputSize);
    return 0;
#endif
    return -1;
}

int lowLevelDecrypt(const char *key, unsigned int keySize, const char *iv, unsigned int ivSize, const char *input, unsigned int inputSize, char *output) {
#if __APPLE__
    SecKeyRef keyRef = keyFromChar(key, keySize);
    CCCryptorStatus status;
    size_t decryptedSize;
    status = CCCrypt(kCCDecrypt, kCCAlgorithmAES, 0, key, keySize, iv, input, inputSize, output, inputSize, &decryptedSize);
    assert(status == kCCSuccess);
    assert(decryptedSize == inputSize);
    return 0;
#endif
    return -1;
}

int lowLevelMaskingEncrypt(const char *key, unsigned int keySize, const char *mask, const char *input, unsigned int inputSize, char *output) {
    //Make sure the data is block size
    assert(keySize % 16 == 0);
#if __APPLE__
    SecKeyRef keyRef = keyFromChar(key, keySize);
    
    char *xorResult = malloc(sizeof(char)*inputSize);
    for (int i = 0; i < inputSize; i+=16) {
        xorResult[i] = input[i] ^ mask[i];
        xorResult[i+1] = input[i+1] ^ mask[i+1];
        xorResult[i+2] = input[i+2] ^ mask[i+2];
        xorResult[i+3] = input[i+3] ^ mask[i+3];
        xorResult[i+4] = input[i+4] ^ mask[i+4];
        xorResult[i+5] = input[i+5] ^ mask[i+5];
        xorResult[i+6] = input[i+6] ^ mask[i+6];
        xorResult[i+7] = input[i+7] ^ mask[i+7];
        xorResult[i+8] = input[i+8] ^ mask[i+8];
        xorResult[i+9] = input[i+9] ^ mask[i+9];
        xorResult[i+10] = input[i+10] ^ mask[i+10];
        xorResult[i+11] = input[i+11] ^ mask[i+11];
        xorResult[i+12] = input[i+12] ^ mask[i+12];
        xorResult[i+13] = input[i+13] ^ mask[i+13];
        xorResult[i+14] = input[i+14] ^ mask[i+14];
        xorResult[i+15] = input[i+15] ^ mask[i+15];
    }
    
    CCCryptorStatus status;
    size_t decryptedSize;
    status = CCCrypt(kCCEncrypt, kCCAlgorithmAES, kCCOptionECBMode, key, keySize, NULL, xorResult, inputSize, output, inputSize, &decryptedSize);
    
    free(xorResult);
    
    assert(status == kCCSuccess);
    assert(decryptedSize == inputSize);
    return 0;
#endif
    return -1;
}

int lowLevelMaskingDecrypt(const char *key, unsigned int keySize, const char *mask, const char *input, unsigned int inputSize, char *output) {
#if __APPLE__
    SecKeyRef keyRef = keyFromChar(key, keySize);
    
    CCCryptorStatus status;
    size_t decryptedSize;
    status = CCCrypt(kCCDecrypt, kCCAlgorithmAES, kCCOptionECBMode, key, keySize, NULL, input, inputSize, output, inputSize, &decryptedSize);
    
    assert(status == kCCSuccess);
    assert(decryptedSize == inputSize);
    
    for (int i = 0; i < inputSize; i+=16) {
        output[i] = output[i] ^ mask[i];
        output[i+1] = output[i+1] ^ mask[i+1];
        output[i+2] = output[i+2] ^ mask[i+2];
        output[i+3] = output[i+3] ^ mask[i+3];
        output[i+4] = output[i+4] ^ mask[i+4];
        output[i+5] = output[i+5] ^ mask[i+5];
        output[i+6] = output[i+6] ^ mask[i+6];
        output[i+7] = output[i+7] ^ mask[i+7];
        output[i+8] = output[i+8] ^ mask[i+8];
        output[i+9] = output[i+9] ^ mask[i+9];
        output[i+10] = output[i+10] ^ mask[i+10];
        output[i+11] = output[i+11] ^ mask[i+11];
        output[i+12] = output[i+12] ^ mask[i+12];
        output[i+13] = output[i+13] ^ mask[i+13];
        output[i+14] = output[i+14] ^ mask[i+14];
        output[i+15] = output[i+15] ^ mask[i+15];
    }
    
    return 0;
#endif
    return -1;
}

void generateSymmetricKey(const char *keyBuf, size_t size) {
#if __APPLE__
    //Conf Dict
    CFStringRef keys[] = {kSecAttrKeyType, kSecAttrKeySizeInBits};
    int keyLenRaw = size*sizeof(char)*8;
    CFNumberRef keyLen = CFNumberCreate(NULL, kCFNumberIntType, &keyLenRaw);
    const void* values[] = { kSecAttrKeyTypeAES, keyLen };
    CFDictionaryRef encDict = CFDictionaryCreate(NULL, keys, values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    CFErrorRef error;
    SecKeyRef key = SecKeyGenerateSymmetric(encDict, &error);
    
    //Copy key
    CFDataRef keyData = convertSecKeyToData(key, kSecFormatRawKey);
    CFDataGetBytes(keyData, CFRangeMake(0, CFDataGetLength(keyData)), keyBuf);
    CFRelease(encDict);
    CFRelease(keyLen);
    CFRelease(keyData);
    CFRelease(key);
#endif
}

void generateRSAKey(const char *privKey, const char *pubKey, size_t size, size_t *privKeySize, size_t *pubKeySize) {
#if __APPLE__
    //Conf Dict
    CFStringRef keys[] = {kSecAttrKeyType, kSecAttrKeySizeInBits};
    int keyLenRaw = size*sizeof(char)*8;
    CFNumberRef keyLen = CFNumberCreate(NULL, kCFNumberIntType, &keyLenRaw);
    const void* values[] = { kSecAttrKeyTypeRSA, keyLen };
    CFDictionaryRef encDict = CFDictionaryCreate(NULL, keys, values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    SecKeyRef pubKeyRef, privKeyRef;
    SecKeyGeneratePair(encDict, &pubKeyRef, &privKeyRef);
    CFRelease(keyLen);
    CFRelease(encDict);
    
    //Copy key
    //CFDataRef pubKeyData = convertSecKeyToData(pubKey);
    //CFDataGetBytes(pubKeyData, CFRangeMake(0, CFDataGetLength(pubKeyData)), pubKey);
    CFDataRef privKeyData = convertSecKeyToData(privKeyRef, kSecFormatPEMSequence);
    CFDataGetBytes(privKeyData, CFRangeMake(0, CFDataGetLength(privKeyData)), privKey);
    *privKeySize = CFDataGetLength(privKeyData);
    //CFRelease(pubKeyData);
    CFRelease(privKeyData);
    CFRelease(pubKeyRef);
    CFRelease(privKeyRef);
    
#endif
}
