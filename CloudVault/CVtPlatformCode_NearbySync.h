//
//  CVtPlatformCode_NearbySync.h
//  CloudVault
//
//  Created by Wai Man Chan on 3/9/17.
//
//

#ifndef CVtPlatformCode_NearbySync_h
#define CVtPlatformCode_NearbySync_h

#include <stdio.h>
#include "CVt_ResourceManagment.h"

typedef void (*CVtPlatformCode_NearbySyncCallbackType)(CVtResourceType type, const char *identifier, const char *obj, size_t objSize);

void CVtPlatformCode_NearbySetCallback(CVtPlatformCode_NearbySyncCallbackType rootCallback);


void CVtPlatformCode_NearbyFetch(CVtResourceType resType, const char *resIdentifier);
void CVtPlatformCode_NearbyCancelFetch(CVtResourceType resType, const char *resIdentifier);

#endif /* CVtPlatformCode_NearbySync_h */
