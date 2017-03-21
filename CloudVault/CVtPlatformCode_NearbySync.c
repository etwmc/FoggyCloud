//
//  CVtPlatformCode_NearbySync.c
//  CloudVault
//
//  Created by Wai Man Chan on 3/9/17.
//
//

#include "CVtPlatformCode_NearbySync.h"

CVtPlatformCode_NearbySyncCallbackType rootCallback;

void CVtPlatformCode_NearbySetCallback(CVtPlatformCode_NearbySyncCallbackType _rootCallback) {
    rootCallback = _rootCallback;
}

void CVtPlatformCode_NearbySync(CVtResourceType resType, const char *resIdentifier) {
    //Fetch the resource
}

void CVtPlatformCode_NearbyCancelFetch(CVtResourceType resType, const char *resIdentifier) {
    
}
