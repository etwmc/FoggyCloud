//
//  CVtPlatformCode_CommunicationDataStructure.h
//  CloudVault
//
//  Created by Wai Man Chan on 3/15/17.
//
//

#ifndef CVtPlatformCode_CommunicationDataStructure_h
#define CVtPlatformCode_CommunicationDataStructure_h

#include "CVt_ResourceManagment_Priv.h"

typedef struct CVtResourceRequestWrapper {
    CVtResourceRequest *request;
    void(*callback)(CVtResourceSource sender, CVtResourceRequest* request, bool success, const char *data, size_t dataLen);
    void *tokens[CVtResourceSource_Count];
    struct CVtResourceRequestWrapper* next;
} CVtResourceRequestWrapper;

#endif /* CVtPlatformCode_CommunicationDataStructure_h */
