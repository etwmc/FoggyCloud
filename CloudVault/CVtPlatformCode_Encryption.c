//
//  CVtPlatformCode_Encryption.c
//  CloudVault
//
//  Created by Wai Man Chan on 2/21/17.
//
//

#include "CVtPlatformCode_Encryption.h"
#include <CommonCrypto/CommonCrypto.h>
#include <Security/Security.h>

#if __APPLE__
#define tranToCFData(varName)  CFDataRef varName##Data = CFDataCreateWithBytesNoCopy(NULL, varName, varName##Size, kCFAllocatorNull);

SecKeyRef darwinKeyFromChar(const char *key, unsigned int keySize) {
    
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
SecKeyRef rsa4096KeyFromString(const char *key) {
    
    //Conf Dict
    CFStringRef keys[] = {kSecAttrKeyType, kSecAttrKeySizeInBits};
    int keyLenRaw = 4096*8;
    
    CFNumberRef keyLen = CFNumberCreate(NULL, kCFNumberSInt32Type, &keyLenRaw);
    const void* values[] = { kSecAttrKeyTypeRSA, keyLen };
    CFDictionaryRef encDict = CFDictionaryCreate(NULL, keys, values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    //Key array to CFData
    size_t keySize = strlen(key);
    tranToCFData(key)
    
    CFErrorRef error = NULL;
    SecKeyRef keyRef = SecKeyCreateFromData(encDict, keyData, &error);
    
    assert(error == NULL);
    
    CFRelease(keyData);
    
    CFRelease(encDict);
    CFRelease(keyLen);
    
    return keyRef;
}
CFDataRef darwinConvertAESSecKeyToData(SecKeyRef key, SecExternalFormat externalFormat) {
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

size_t CVtPlatformCode_Encryption_AES256_CFB(const char *content, size_t contentLen, const char *key, const char *iv, char **output) {
#if __APPLE__
    SecKeyRef keyRef = darwinKeyFromChar(key, 32);
    
    CCCryptorStatus status;
    size_t newSize = contentLen+16;
    *output = malloc(newSize);
    status = CCCrypt(kCCEncrypt, kCCAlgorithmAES, kCCOptionPKCS7Padding, key, 32, iv, content, contentLen, *output, newSize, &newSize);
    
    if (status != kCCSuccess) {
        free(*output);
        *output = NULL;
        return -2;
    }
    
    return newSize;
#endif
    //Not implemented
    return -1;
}

size_t CVtPlatformCode_Decryption_AES256_CFB(const char *ciphertext, size_t ciphertextLen, const char *key, const char *iv, char **output) {
#if __APPLE__
    SecKeyRef keyRef = darwinKeyFromChar(key, 32);
    
    CCCryptorStatus status;
    size_t newSize = ciphertextLen+16;
    *output = malloc(newSize);
    status = CCCrypt(kCCEncrypt, kCCAlgorithmAES, kCCOptionPKCS7Padding, key, 32, iv, ciphertext, ciphertextLen, *output, newSize, &newSize);
    
    if (status != kCCSuccess) {
        free(*output);
        *output = NULL;
        return -2;
    }
    
    return newSize;
#endif
    //Not implemented
    return -1;
}

int CVtPlatformCode_Encryption_RSA4096(const char *plaintext, size_t platintextLen, const char *publicKey, char *ciphertext) {
#if __APPLE__
    
#endif
    return -1;
}

int CVtPlatformCode_Decryption_RSA4096(const char *ciphertext, size_t ciphertextLen, const char *publicKey, char *output) {
#if __APPLE__
#endif
    return -1;
}

void CVtPlatformCode_Hash_SHA512(const char *plaintext, size_t plaintextLen, char *output) {
    CC_SHA512(plaintext, plaintextLen, output);
}

void CVtPlatformCode_SafeStorage_Set(const char *keyOwner, const char *keyType, const char *content, size_t contentSize) {
    
}
size_t CVtPlatformCode_SafeStorage_Get(const char *keyOwner, const char *keyType, const char **content) {
    return -1;
}
