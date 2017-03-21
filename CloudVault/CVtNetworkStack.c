//
//  CVtNetworkStatistic.c
//  CloudVault
//
//  Created by Wai Man Chan on 2/7/17.
//
//

#include "CVtNetworkStack.h"
#include "CVtPlatformCode_Threading.h"
#include "CVtPlatformCode_Network.h"
#include <stdlib.h>
#include <strings.h>

typedef struct CVtNetwork_httpRequestNode {
    CVtNetwork_httpRequest request;
    void *token;
    CVtNetwork_ResponseCallback callback;
    struct CVtNetwork_httpRequestNode *next;
} CVtNetwork_httpRequestNode;

//Global definition for network pool
CVtthreading_pool networkPool;
CVtsemaphore semaphore;
CVtNetwork_httpRequestNode *rootNode;
CVtNetwork_httpRequestNode *lastNode;

CVtsemaphore counting;


CVtNetwork_httpRequest CVtNetworkStack_requestCreate(CVtNetwork_httpMethod method, const char *path, const CVtNetwork_httpRequestHeader *header, size_t headerSize, const char *body, size_t bodySize) {
    CVtNetwork_httpRequest request;
    request.method = method;
    request.body = malloc(bodySize);
    request.header = malloc(sizeof(CVtNetwork_httpRequestHeader)*headerSize);
    bcopy(body, (char*)request.body, bodySize);
    //Copy headers
    for (int i = 0; i < headerSize; i++) {
        char *ptr = ((char*)request.header[i].header);
        ptr = malloc(strlen(header[i].header)+1);
        bcopy(header[i].header, ptr, strlen(header[i].header)+1);
        
        ptr = ((char*)request.header[i].value);
        ptr = malloc(strlen(header[i].value)+1);
        bcopy(header[i].value, ptr, strlen(header[i].value)+1);
    }
    request.bodySize = bodySize;
    request.headerSize = headerSize;
    return request;
}
void CVtNetworkStack_requestRelease(CVtNetwork_httpRequest request) {
    free(request.body);
    for (int i = 0; i < request.headerSize; i++) {
        free(request.header[i].header);
        free(request.header[i].value);
    }
    free(request.header);
}

void CVtNetworkStack_callback(callbackArg args, CVtNetwork_httpResponse result) {
    //Sanity check
    if (args.argLen == sizeof(CVtNetwork_httpRequestNode)) {
        CVtNetwork_httpRequestNode *node = args.args;
        node->callback(node->request, result, node->token);
        free(node);
    }
}

void CVtNetworkStack_spinoff() {
    while (true) {
        CVtsemaphoreLock(counting);
        CVtsemaphoreLock(semaphore);
        //Secure, so take a node
        CVtNetwork_httpRequestNode *node = rootNode;
        rootNode = rootNode->next;
        //Sanity check
        if (node) {
            callbackArg returnArg;
            returnArg.args = node;
            returnArg.argLen = sizeof(CVtNetwork_httpRequestNode);
            CVtNetwork_HTTPRequestToAddr(node->request, &CVtNetworkStack_callback, returnArg);
        }
        CVtsemaphoreUnlock(semaphore);
    }
}

void CVtNetworkStack_init() {
    networkPool = CVtthreading_poolCreate("networkStack");
    //Spin off the pool
    counting = CVtsemaphoreCreate(0);
    CVtthreading_performAsyncNoPara(networkPool, CVtNetworkStack_spinoff);
    //Queuing point
    semaphore = CVtsemaphoreCreate(1);
}

bool CVtNetworkStack_hasPendingRequests() {
    CVtsemaphoreLock(semaphore);
    bool result = rootNode != NULL;
    CVtsemaphoreUnlock(semaphore);
    return result;
}

void CVtNetworkStack_parkRequest(CVtNetwork_httpRequest request, void *responseToken, CVtNetwork_ResponseCallback callback) {
    //Generate a node
    CVtNetwork_httpRequestNode *newNode = malloc(sizeof(CVtNetwork_httpRequestNode));
    newNode->next = NULL;
    newNode->request = request;
    newNode->callback = callback;
    newNode->token = responseToken;
    
    CVtsemaphoreLock(semaphore);
    if (rootNode != NULL) {
        //There is existing request, so extend the list
        lastNode->next = newNode;
        lastNode = newNode;
    } else {
        //There is nothing
        lastNode = newNode;
        rootNode = newNode;
    }
    CVtsemaphoreUnlock(semaphore);
    CVtsemaphoreUnlock(counting);
}

void CVtNetworkStack_addRoundTrip(float roundTrip) {}
float CVtNetworkStack_roundTripUpperLimit() { return 0; }
float CVtNetworkStack_roundTripLowerLimit() { return 0; }
