//
//  CVt_ResourceManagment.c
//  CloudVault
//
//  Created by Wai Man Chan on 3/9/17.
//
//

#include "CVt_ResourceManagment_Priv.h"
#include "CVt_FileManagement.h"
#include <assert.h>
#include <limits.h>
#include "CVtProjectConstant.h"
#include <string.h>
#include <stdlib.h>

struct CVtResourceProvider {
    void (*registerRequest)(CVtResourceRequest *request, void(*stateCallback)(CVtResourceSource sender, CVtResourceRequest* request, bool success, const char *data, size_t dataLen));
    void (*cancelRequest)(CVtResourceRequest *request);
};

struct CVtResourceProvider resProvider[] = {};

typedef struct CVtResourceRemoteRequest {
    CVtResourceRequest request;
    
    CVtsemaphore writeSemaphore;
    //Signal when one of the source get the correct
    CVtsemaphore completionSeamphore;
    bool finished;
    
    const char **dataPtr;
    size_t dataLen;
    
    CVtResourceSource finalSource;
} CVtResourceRemoteRequest;

void CVt_ResourceManagmentFinish(CVtResourceSource sender, CVtResourceRequest *request, bool success, const char *data, size_t dataLen) {
    CVtResourceRemoteRequest *remoteRequest = (CVtResourceRemoteRequest *)request;
    if (success) {
        
        //Clean up
        for (int i = 0; i < CVtResourceSource_Count; i++) {
            //Register request
            resProvider[i].cancelRequest((CVtResourceRequest*)&request);
        }
        
        //Line up
        CVtsemaphoreLock(remoteRequest->writeSemaphore);
        //First one in being used
        if (!remoteRequest->finished) {
            remoteRequest->finished = true;
            remoteRequest->finalSource = sender;
            
            //Read back
            *remoteRequest->dataPtr = malloc(dataLen);
            bcopy(data, (char*)*remoteRequest->dataPtr, dataLen);
            remoteRequest->dataLen = dataLen;
            
            CVtsemaphoreUnlock(remoteRequest->completionSeamphore);
        }
        CVtsemaphoreUnlock(remoteRequest->writeSemaphore);
    }
}

size_t CVt_ResourceManagmentRequest(CVtResourceType type, const char *identifier, const char **data) {
    static char prefix[PATH_MAX];
    switch (type) {
        case CVtResourceType_transcationalFile:
            strncpy(prefix, CVt_fileAddr(CVtProjectConstant_fileAssetFdName), PATH_MAX);
            break;
            
        default:
            assert(false);
            break;
    }
    if (CVt_FileManagmentRequestCanFulfill(prefix, identifier)) {
        //Request at the file manager
        return CVt_rawReadback(data, prefix, identifier);
    } else {
        //Request from other sources
        CVtResourceRemoteRequest request = {
            type, identifier, CVtsemaphoreCreate(1), CVtsemaphoreCreate(0), false, data
        };
        
        for (int i = 0; i < CVtResourceSource_Count; i++) {
            //Register request
            resProvider[i].registerRequest((CVtResourceRequest*)&request, &CVt_ResourceManagmentFinish);
        }
        
        //Wait
        CVtsemaphoreLock(request.completionSeamphore);
        return request.dataLen;
    }
}
