//
//  CVtKeychain.c
//  CloudVault
//
//  Created by Wai Man Chan on 3/4/17.
//
//

#include "CVtKeychain.h"
#include "CVtNetworkStack.h"
#include "CVtProjectConstant.h"
#include "CVt_FileManagement.h"
#include <strings.h>
#include <assert.h>
#include "CVtPlatformCode_Encryption.h"
#include "CVtPlatformCode_Threading.h"

userID CVt_selfUserID() {
    userID id;
    size_t s = CVt_rawReadback(id.UUID, CVt_fileAddr(CVtProjectConstant_fileRoot), "identity");
    assert(s == 36);
    return id;
}

bool CVt_userIsSelf(userID user) {
    userID id = CVt_selfUserID();
    int res = bcmp(user.UUID, id.UUID, sizeof(userID));
    return res == 0;
}

typedef struct {
    userID user;
    CVtsemaphore semaphore;
} keychainRequestToken;
void keychainRequestBack(CVtNetwork_httpRequest request, CVtNetwork_httpResponse response, void *token) {
    if (response.statusCode >= 200 && response.statusCode < 300) {
        //The result is good
        //Cache the key
        CVt_rawFlushDown(response.body, response.bodyLen, CVt_fileAddr(CVtProjectConstant_fileCacheKeyFdName), ((keychainRequestToken*)token)->user.UUID);
        //Unlock the semaphore
        CVtsemaphoreUnlock(((keychainRequestToken*)token)->semaphore);
    } else {
        //Try again
        CVtNetworkStack_parkRequest(request, token, &keychainRequestBack);
    }
}

size_t CVt_getPublicKey(userID user, char **publicKey) {
    size_t cacheSize = CVt_rawReadback(publicKey, CVt_fileAddr(CVtProjectConstant_fileCacheKeyFdName), user.UUID);
    if (cacheSize >= 0) {
        //Cache hit
        return cacheSize;
    }
    //Has to read from server
    const char path[1024];
    snprintf(path, 1024, CVt_stringMerge(CVt_dbBackendURL(CVtProjectConstant_dbUserKeyDir), "?tuid=%s"), user.UUID);
    
    CVtsemaphore waitForPublicKey = CVtsemaphoreCreate(0);
    CVtNetwork_httpRequest request = CVtNetworkStack_requestCreate(CVtNetwork_httpMethodGET, path, NULL, 0, NULL, 0);
    
    keychainRequestToken token;
    token.semaphore = waitForPublicKey;
    token.user = user;
    
    CVtNetworkStack_parkRequest(request, &token, &keychainRequestBack);
    CVtsemaphoreLock(waitForPublicKey);
    //Once unlock, the semaphore is finished
    CVtsemaphoreRelease(waitForPublicKey);
    //The key is cached, so read from the cache
    return CVt_rawReadback(publicKey, CVt_fileAddr(CVtProjectConstant_fileCacheKeyFdName), user.UUID);
}

size_t CVt_getPrivateKey(userID user, char **privateKey) {
    assert(userIsSelf(user));
    //You can only load your own private key
    return CVtPlatformCode_SafeStorage_Get(user.UUID, "PrivateRSA4096", privateKey);
}

void securelyFree(void *ptr, size_t size) {
    bzero(ptr, size);
    free(ptr);
}
