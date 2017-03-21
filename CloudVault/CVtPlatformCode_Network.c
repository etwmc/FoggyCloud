//
//  CVtPlatformCode_Network.c
//  CloudVault
//
//  Created by Wai Man Chan on 2/6/17.
//
//

#include "CVtPlatformCode_Network.h"
#include "CVtPlatformCode_CommunicationDataStructure.h"
#include "CVtPlatformCode_Threading.h"
#include "CVtProjectConstant.h"
#if __APPLE__
#include "CVtPlatformCode_NetworkDarwinHelper.h"
#endif

static const char CVtNetwork_httpMethodStr_get[] = "GET";
static const char CVtNetwork_httpMethodStr_post[] = "POST";
static const char CVtNetwork_httpMethodStr_put[] = "PUT";
static const char CVtNetwork_httpMethodStr_del[] = "DELETE";

const char const *methodToStr(CVtNetwork_httpMethod m) {
    switch (m) {
        case CVtNetwork_httpMethodGET: return CVtNetwork_httpMethodStr_get;
        case CVtNetwork_httpMethodPUT: return CVtNetwork_httpMethodStr_put;
        case CVtNetwork_httpMethodPOST: return CVtNetwork_httpMethodStr_post;
        case CVtNetwork_httpMethodDELETE: return CVtNetwork_httpMethodStr_del;
    }
}

void CVtNetwork_HTTPRequestToAddr(CVtNetwork_httpRequest request, networkCallback callback, callbackArg arg) {
#if __APPLE__
    //Just pass to the Obj-C subler
    addNewRequest(request, callback, arg);
#endif
}

CVtsemaphore CVtNetwork_rootLock;
CVtResourceRequestWrapper *CVtNetwork_root;

void CVtNetwork_HTTPInit() {
    CVtNetwork_root = NULL;
    CVtNetwork_rootLock = CVtsemaphoreCreate(1);
}

void CVtNetwork_HTTPRequestCallback(callbackArg args, CVtNetwork_httpResponse result) {
    CVtsemaphoreLock(CVtNetwork_rootLock);
    
    //Get the object
    if (args.args == CVtNetwork_root) {
        //Take out the root
        CVtNetwork_root = CVtNetwork_root->next;
    } else {
        //Take out a child
        CVtResourceRequestWrapper *wrapper = CVtNetwork_root;
        while (wrapper != NULL) {
            if (wrapper->next == args.args) {
                //Take down
                wrapper->next = wrapper->next->next;
                break;
            }
            wrapper = wrapper->next;
        }
    }
    CVtsemaphoreUnlock(CVtNetwork_rootLock);
    
    CVtResourceRequestWrapper *wrapper = args.args;
    bool success = result.statusCode >= 200 && result.statusCode < 300;
    wrapper->callback(CVtResourceSource_Backend, wrapper->request, success, result.body, result.bodyLen);
};

void CVtNetwork_HTTPRegisterRequest(CVtResourceRequest *request, void(*stateCallback)(CVtResourceSource sender, CVtResourceRequest* request, bool success, const char *data, size_t dataLen)) {
    CVtsemaphoreLock(CVtNetwork_rootLock);
    CVtResourceRequestWrapper *newWrapper = malloc(sizeof(CVtResourceRequestWrapper));
    newWrapper->request = request;
    newWrapper->callback = stateCallback;
    newWrapper->next = CVtNetwork_root;
    CVtNetwork_root = newWrapper;
    
    //Translate resource reqyest to HTTP request
    const char path[1024];
    switch (request->type) {
        case CVtResourceType_transcationalFile:
            snprintf(path, 1024, CVt_stringMerge(CVt_dbBackendURL(CVtProjectConstant_dbMsgJourDir), "%s?mid=%s"), request->identifier);
            break;
        default:
            break;
    }
    CVtNetwork_httpRequest httpRequest = CVtNetworkStack_requestCreate(CVtNetwork_httpMethodGET, path, NULL, 0, NULL, 0);
    callbackArg arg;
    arg.args = newWrapper;
    arg.argLen = sizeof(CVtResourceRequestWrapper);
    
    
    //Sumbit
    CVtNetwork_HTTPRequestToAddr(httpRequest, &CVtNetwork_HTTPRequestCallback, arg);
    
    CVtsemaphoreUnlock(CVtNetwork_rootLock);
}

void CVtNetwork_HTTPCancelRequest(CVtResourceRequest *request) {
    //cancelRequest(<#void *requestToken#>)
}
