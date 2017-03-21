//
//  CVtPlatformCode_Network.h
//  CloudVault
//
//  Created by Wai Man Chan on 2/6/17.
//
//

#ifndef CVtPlatformCode_Network_h
#define CVtPlatformCode_Network_h

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "CVtNetworkStack.h"

typedef struct callbackArg {
    void *args;
    size_t argLen;
} callbackArg;

typedef void (*networkCallback)(callbackArg args, CVtNetwork_httpResponse result);

/*
 General Network Function
 */

void CVtNetwork_HTTPRequestToAddr(CVtNetwork_httpRequest request, networkCallback callback, callbackArg arg);
const char *methodToStr(CVtNetwork_httpMethod m);

#endif /* CVtPlatformCode_Network_h */
