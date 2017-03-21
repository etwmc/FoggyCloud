//
//  CVtCloudTranscation.c
//  CloudVault
//
//  Created by Wai Man Chan on 2/6/17.
//
//

#include "CVtCloudTranscation_Priv.h"
#include <stdlib.h>
#include "CVtPlatformCode_Network.h"
#include "CVtProjectConstant.h"
#include "CVtNetworkStack.h"
#include "CVt_LocalJournal.h"
#include "CVt_FileManagement.h"
#include <string.h>
#include "CVtPlatformCode_Encryption.h"
#include <assert.h>
#include "base64.h"

typedef struct {
    size_t length;
    unsigned char version;
    CVtOperationPointer op;
} transcationBlock;

typedef struct transcationalOperation {
    CVtOperationPrototype operation;
    CVtOperationPrototype rollback;
    struct transcationalOperation *nextVersion;
    struct transcationalOperation *nextOperation;
} transcationalOperation;

unsigned int numberOfVersion = 0;
unsigned int numberOfNewestProtocol = 0;
transcationalOperation *rootPtr = NULL;
transcationalOperation *currentPtr = NULL;
transcationalOperation *currentLvFirstPtr = NULL;
transcationalOperation *preivousLvPtr = NULL;

CVtOperationPointer registerOperation(CVtOperationPrototype operation, CVtOperationPrototype rollback) {
    transcationalOperation *op = malloc(sizeof(transcationalOperation));
    op->operation = operation;
    op->rollback = rollback;
    op->nextOperation = NULL;
    op->nextVersion = NULL;
    if (rootPtr == NULL) {
        //Nothing has defined
        rootPtr = op;
        currentLvFirstPtr = op;
    } else {
        //Has estalishedStuff
        currentPtr->nextOperation = op;
        if (preivousLvPtr) {
            preivousLvPtr->nextVersion = op;
            currentLvFirstPtr = op;
            preivousLvPtr = NULL;
        } else {
            //Not the beginning of the current level
            //Nothing
        }
        
    }
    currentPtr = op;
    CVtOperationPointer ptr;
    ptr.protocolVersion = numberOfVersion;
    ptr.opIndex = numberOfNewestProtocol;
    numberOfNewestProtocol++;
    return ptr;
}

transcationalOperation *operationForPointer(CVtOperationPointer ptr) {
    //Lock in version
    transcationalOperation *opPtr = rootPtr;
    for (int i = 0; i < ptr.protocolVersion; i++) {
        opPtr = opPtr->nextVersion;
        if (opPtr == NULL) return NULL;
    }
    for (int i = 0; i < ptr.opIndex; i++) {
        opPtr = opPtr->nextOperation;
        if (opPtr == NULL) return NULL;
    }
    return opPtr;
}

void _newProtocolVersion() {
    numberOfVersion++;
    numberOfNewestProtocol = 0;
    preivousLvPtr = currentLvFirstPtr;
    currentLvFirstPtr = NULL;
}

struct CVt_transcationalCallbackToken {
    UUID transcationUUID;
    CVtOperationCallback callback;
};

void CVt_procedCallback(CVtNetwork_httpRequest request, CVtNetwork_httpResponse response, void *token);

UUID issueOperation(CVtOperationPointer operationPtr, void *args, size_t argsLen, CVtOperationCallback callback) {
    UUID transcationUUID;
    for (int i = 0; i < 16; i++) {
        transcationUUID.data[i] = rand();
    }
    const char *transcationUUIDStr = uuidToStr(transcationUUID);
    
    //Assemble transational packet: 256b RSA encrypted AES key, 16B AES IV in plaintext, and the operation pointer, rest of argument and their combined SHA512 in PKCS7 AES 256
    char *packet = malloc(sizeof(operationPtr)+argsLen+64+8+48);
    size_t packetSize = 0;
    for (int i = 0; i < 48; i++) {
        packet[i] = rand();
    }
    {
        void *data = malloc(sizeof(operationPtr)+argsLen+64);
        bcopy(&operationPtr, data, sizeof(operationPtr));
        bcopy(args, data+sizeof(operationPtr), argsLen);
        //Do the hashing
        char dataHash[64];
        CVtPlatformCode_Hash_SHA512(data, sizeof(operationPtr)+argsLen, dataHash);
        {
            //Encrypt the hashing to verify the sender
            const char *selfPrivateKey;
            size_t keySize = CVt_getPrivateKey(CVt_selfUserID(), &selfPrivateKey);
            CVtPlatformCode_Decryption_RSA4096(dataHash, 64, selfPrivateKey, data+sizeof(operationPtr)+argsLen);
            securelyFree(selfPrivateKey, keySize);
        }
        //Encrypt the package
        char *ciphertext;
        size_t s = CVtPlatformCode_Encryption_AES256_CFB(data, sizeof(operationPtr)+argsLen+64, packet, packet+32, &ciphertext);
        free(data);
        //Add to the packet
        packetSize = s+48;
        bcopy(ciphertext, packet+48, s);
    }
    
    {
        //Read in public key
        const char *selfPubliceKey;
        size_t keySize = CVt_getPublicKey(CVt_selfUserID(), &selfPubliceKey);
        //Encrypt the packet key
        char wrappedKey[32];
        CVtPlatformCode_Encryption_RSA4096(packet, 32, selfPubliceKey, wrappedKey);
        //Replace the key
        bcopy(wrappedKey, packet, 32);
        securelyFree(selfPubliceKey, keySize);
    }
    
    //Write the packet in the disk
    CVt_rawFlushDown(packet, packetSize, CVt_fileAddr(CVtProjectConstant_fileAssetFdName), transcationUUIDStr);
    
    //To prevenet power loss corruption, issue an event to journal
    writeToLocalJournal(transcationUUID.data, sizeof(UUID));
    
    //Callback token
    struct CVt_transcationalCallbackToken *token = malloc(sizeof(struct CVt_transcationalCallbackToken));
    token->transcationUUID = transcationUUID;
    token->callback = callback;
    
    //Hash
    char hash[64];
    CVtPlatformCode_Hash_SHA512(packet, packetSize, hash);
    
    //Need to wrap the hash with self public key to sign the body
    char wrappedHash[64];
    {
        const char *selfPublicKeyDoc;
        size_t keySize = CVt_getPublicKey(CVt_selfUserID(), &selfPublicKeyDoc);
        //Encrypt the packet key
        CVtPlatformCode_Encryption_RSA4096(packet, 64, selfPublicKeyDoc, hash);
        //Replace the key
        securelyFree(selfPublicKeyDoc, keySize);
    }
    
    //Base64 hash
    //Len = 63 char->84 char, 1 char->2 char, 1 for plaintext ending->87
    char base64Hash[87];
    Base64encode(base64Hash, wrappedHash, 64);
    
    //Check size, should it be send as a part
    bool bodyTranscation = false;
    if (bodyTranscation) {
        assert(false);
        //Not implemented yet
    } else {
        unsigned int headerSize = 0;
        CVtNetwork_httpRequestHeader headers[] = {
            {"tuid", CVt_selfUserID().UUID },
            {"body-signature", base64Hash },
            {"msgID", transcationUUIDStr },
                                                  };
        CVtNetwork_httpRequest request = CVtNetworkStack_requestCreate(CVtNetwork_httpMethodPOST, CVt_dbBackendURL(CVtProjectConstant_dbQueueDir), headers, headerSize, args, argsLen);
        
        CVtNetworkStack_parkRequest(request, token, &CVt_procedCallback);
    }
    
    free(transcationUUIDStr);
    return transcationUUID;
}

UUID issueRemoteOperation(userID targetUser, CVtOperationPointer operationPtr, void *args, size_t argsLen, CVtOperationCallback callback) {
    if (CVt_userIsSelf(targetUser)) {
        //Sending message to self, so used the other approach (if continue, the packet encryption key will not be wrapped, hence causing a security flaw
        return issueOperation(operationPtr, args, argsLen, callback);
    }
    
    UUID transcationUUID;
    for (int i = 0; i < 16; i++) {
        transcationUUID.data[i] = rand();
    }
    const char *transcationUUIDStr = uuidToStr(transcationUUID);
    
    //Assemble transational packet: 256b RSA encrypted AES key, 16B AES IV in plaintext, and the operation pointer, rest of argument and their combined SHA512 in PKCS7 AES 256
    char *packet = malloc(sizeof(operationPtr)+argsLen+64+8+48);
    size_t packetSize = 0;
    for (int i = 0; i < 48; i++) {
        packet[i] = rand();
    }
    {
        void *data = malloc(sizeof(operationPtr)+argsLen+64);
        bcopy(&operationPtr, data, sizeof(operationPtr));
        bcopy(args, data+sizeof(operationPtr), argsLen);
        //Do the hashing
        char dataHash[64];
        CVtPlatformCode_Hash_SHA512(data, sizeof(operationPtr)+argsLen, dataHash);
        {
            //Encrypt the hashing to verify the sender
            const char *selfPrivateKey;
            size_t keySize = CVt_getPrivateKey(CVt_selfUserID(), &selfPrivateKey);
            CVtPlatformCode_Decryption_RSA4096(dataHash, 64, selfPrivateKey, data+sizeof(operationPtr)+argsLen);
            securelyFree(selfPrivateKey, keySize);
        }
        //Encrypt the package
        char *ciphertext;
        size_t s = CVtPlatformCode_Encryption_AES256_CFB(data, sizeof(operationPtr)+argsLen+64, packet, packet+32, &ciphertext);
        free(data);
        //Add to the packet
        packetSize = s+48;
        bcopy(ciphertext, packet+48, s);
    }
    
    {
        char tempKey[32];
        //Read in user public key
        const char *targetPublicKey;
        size_t targetPubKeyLen = CVt_getPublicKey(CVt_selfUserID(), &targetPublicKey);
        CVtPlatformCode_Encryption_RSA4096(packet, 32, targetPublicKey, tempKey);
        //Replace the key
        securelyFree(targetPublicKey, targetPubKeyLen);
        
        const char *selfPrivateKey;
        size_t selfPriKeyLen = CVt_getPrivateKey(CVt_selfUserID(), &selfPrivateKey);
        //Encrypt the packet key
        CVtPlatformCode_Decryption_RSA4096(tempKey, 32, selfPrivateKey, packet);
        //Replace the key
        securelyFree(selfPrivateKey, selfPriKeyLen);
    }
    
    //Write the packet in the disk
    CVt_rawFlushDown(packet, packetSize, CVt_fileAddr(CVtProjectConstant_fileAssetFdName), transcationUUIDStr);
    
    //To prevenet power loss corruption, issue an event to journal
    writeToLocalJournal(transcationUUID.data, sizeof(UUID));
    
    //Callback token
    struct CVt_transcationalCallbackToken *token = malloc(sizeof(struct CVt_transcationalCallbackToken));
    token->transcationUUID = transcationUUID;
    token->callback = callback;
    
    //Hash
    char hash[64];
    CVtPlatformCode_Hash_SHA512(packet, packetSize, hash);
    
    //Need to wrap the hash with private key to sign the body
    char wrappedHash[64];
    {
        const char *selfPrivateKeyDoc;
        size_t keySize = CVt_getPrivateKey(CVt_selfUserID(), &selfPrivateKeyDoc);
        //Encrypt the packet key
        CVtPlatformCode_Decryption_RSA4096(packet, 64, selfPrivateKeyDoc, hash);
        //Replace the key
        securelyFree(selfPrivateKeyDoc, keySize);
    }
    
    //Base64 hash
    //Len = 63 char->84 char, 1 char->2 char, 1 for plaintext ending->87
    char base64Hash[87];
    Base64encode(base64Hash, wrappedHash, 64);
    
    //Check size, should it be send as a part
    bool bodyTranscation = false;
    if (bodyTranscation) {
        assert(false);
        //Not implemented yet
    } else {
        unsigned int headerSize = 0;
        CVtNetwork_httpRequestHeader headers[] = {
            {"tuid", targetUser.UUID },
            {"body-signature", base64Hash },
            {"msgID", transcationUUIDStr },
        };
        CVtNetwork_httpRequest request = CVtNetworkStack_requestCreate(CVtNetwork_httpMethodPOST, CVt_dbBackendURL(CVtProjectConstant_dbQueueDir), headers, headerSize, args, argsLen);
        
        CVtNetworkStack_parkRequest(request, token, &CVt_procedCallback);
    }
    
    free(transcationUUIDStr);
    return transcationUUID;
}

//From directly sumbit to the queue, so no user checking needed
void CVt_procedCallback(CVtNetwork_httpRequest request, CVtNetwork_httpResponse response, void *token) {
    struct CVt_transcationalCallbackToken *_token = token;
    UUID tUUID = _token->transcationUUID;
    const char *tUUIDStr = uuidToStr(tUUID);
    if (response.statusCode >= 200 && response.statusCode < 300) {
        //Success update to the backend
        
        //Read the arguments
        void *wrappedArgs;
        size_t wrappedArgLen = CVt_offsetReadback(&wrappedArgs, sizeof(CVtOperationPointer), CVt_fileAddr(CVtProjectConstant_fileAssetFdName), tUUIDStr);
        //Key unwrap
        char key[32];
        {
            const char *publicKey;
            size_t keySize = CVt_getPublicKey(CVt_selfUserID(), &publicKey);
            CVtPlatformCode_Encryption_RSA4096(wrappedArgs, 32, publicKey, key);
            securelyFree(publicKey, keySize);
        }
        //Decrypt the packet
        char *packet;
        size_t packetSize;
        {
            packetSize = CVtPlatformCode_Decryption_AES256_CFB(wrappedArgs+48, wrappedArgLen-48, key, wrappedArgs+32, &packet);
            free(wrappedArgs);
            bzero(key, 32);
        }
        //Check checksum
        {
            char checksum[64];
            CVtPlatformCode_Hash_SHA512(packet, packetSize-64, checksum);
            bool valid = bcmp(checksum, packet+packetSize-64, 64);
            if (!valid) {
                free(packet);
                goto cleanup;
            }
        }
        
        //Read back the operation pointer
        CVtOperationPointer *operationPtr = packet;
        transcationalOperation *operation = operationForPointer(*operationPtr);
        
        if (operation == NULL) {
            //The transcation is not recogitized
            //Yield an error
            assert(false);
            free(packet);
            goto cleanup;
        }
        
        //Run the operation
        CVtOperationStatus status = operation->operation(packet+sizeof(CVtOperationPointer), packetSize-sizeof(CVtOperationPointer), false);
        //Callback
        CVtOperationCallback callback = _token->callback;
        CVtOperationCallbackResponse response = callback(tUUID, status);
        if (status == CVtOperationStatus_Conflict && response == CVtOperationCallbackResponse_PassThr) {
            //Retry with ignore condition
            status = operation->operation(packet+sizeof(CVtOperationPointer), packetSize-sizeof(CVtOperationPointer), true);
            //Under this 
            response = callback(tUUID, status);
        }
    }
cleanup:
    free(_token);
    free(tUUIDStr);
    return;
}

void CVt_processRemoteTranscation(CVt_RemoteUUIDString versionUUID) {
    
}
