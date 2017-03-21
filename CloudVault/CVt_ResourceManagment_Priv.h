//
//  CVt_ResourceManagment_Priv.h
//  CloudVault
//
//  Created by Wai Man Chan on 3/9/17.
//
//

#ifndef CVt_ResourceManagment_Priv_h
#define CVt_ResourceManagment_Priv_h

#include "CVt_ResourceManagment.h"

typedef struct CVtResourceRequest {
    CVtResourceType type;
    const char *identifier;
} CVtResourceRequest;

typedef enum {
    CVtResourceSource_Fail = -1,
    CVtResourceSource_Nearby = 0,
    CVtResourceSource_Backend,
    CVtResourceSource_Count,
} CVtResourceSource;

void CVtNetwork_HTTPInit();

#endif /* CVt_ResourceManagment_Priv_h */
