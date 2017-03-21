//
//  CVtNetworkStatistic.h
//  CloudVault
//
//  Created by Wai Man Chan on 2/7/17.
//
//

#ifndef CVtNetworkStack_h
#define CVtNetworkStack_h

#include <stdio.h>
#include <stdbool.h>

/*
 HTTP 1 types
 */
typedef enum {
    CVtNetwork_httpMethodGET,
    CVtNetwork_httpMethodPOST,
    CVtNetwork_httpMethodPUT,
    CVtNetwork_httpMethodDELETE,
} CVtNetwork_httpMethod;
typedef struct CVtNetwork_httpRequestHeader {
    const char *header; const char *value;
} CVtNetwork_httpRequestHeader;
typedef struct CVtNetwork_httpRequest {
    CVtNetwork_httpMethod method;
    const char *path;
    const CVtNetwork_httpRequestHeader *header; size_t headerSize;
    const char *body;   size_t bodySize;
}CVtNetwork_httpRequest;
typedef struct httpResponse {
    int statusCode;
    const CVtNetwork_httpRequestHeader *header; size_t headerSize;
    const char *body;   size_t bodyLen;
} CVtNetwork_httpResponse;
typedef void (*CVtNetwork_ResponseCallback)(CVtNetwork_httpRequest request, CVtNetwork_httpResponse response, void *token);
bool CVtNetworkStack_hasPendingRequests();
void CVtNetworkStack_parkRequest(CVtNetwork_httpRequest request, void *responseToken, CVtNetwork_ResponseCallback callback);

//Object Lifecycle
CVtNetwork_httpRequest CVtNetworkStack_requestCreate(CVtNetwork_httpMethod method, const char *path, const CVtNetwork_httpRequestHeader *header, size_t headerSize, const char *body, size_t bodySize);
void CVtNetworkStack_requestRelease(CVtNetwork_httpRequest request);

/*
 * Optimization statistic
 */

void CVtNetworkStack_addRoundTrip(float roundTrip);
float CVtNetworkStack_roundTripUpperLimit();
float CVtNetworkStack_roundTripLowerLimit();


void CVtNetworkStack_init();

#endif /* CVtNetworkStatistic_h */
