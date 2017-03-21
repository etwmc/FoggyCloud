//
//  CVt_ResourceManagment.h
//  CloudVault
//
//  Created by Wai Man Chan on 3/9/17.
//
//

#ifndef CVt_ResourceManagment_h
#define CVt_ResourceManagment_h

#include <stdio.h>
#include "CVtPlatformCode_Threading.h"

typedef enum {
    CVtResourceType_transcationalFile = 0,
    CVtResourceType_count
} CVtResourceType;

size_t CVt_ResourceManagmentRequest(CVtResourceType type, const char *identifier, const char **data);

#endif /* CVt_ResourceManagment_h */
