//
//  CVtEventualMerger.c
//  CloudVault
//
//  Created by Wai Man Chan on 3/6/17.
//
//

#include "CVtEventualMerger.h"
#include <mjson.h>
#include "CVtCloudTranscation_Priv.h"

int CVt_currentHardVersion();

void CVt_pushHandle(void *message) {
    int count;
    int dependentVersion;
    struct CVt_RemoteUUIDString uuids[64];
    
    const struct json_attr_t CVt_pushJSONUUID[] = {
        {"id",   t_string,    STRUCTOBJECT(struct CVt_RemoteUUIDString, str),
            .len = sizeof(uuids)},
        {NULL},
    };
    
    const struct json_attr_t CVt_pushJSONPayload[] = {
        {"dependent", t_uinteger, .addr.integer = &dependentVersion},
        {"UUIDs",   t_array,    STRUCTARRAY(uuids, CVt_pushJSONUUID, &count)},
        {NULL},
    };
    
    //Parse
    int status = json_read_object(message, CVt_pushJSONPayload, NULL);
    //Check version and parsing
    if (status != 0 || CVt_currentHardVersion() != dependentVersion) {
        //Either the parsing is incorrect, or the version is not correct to directly update
        //So, query everything
    } else {
        //Perform each of the request, then check update
        for (int i = 0; i < count; i++) {
            CVt_processRemoteTranscation(uuids[i]);
        }
    }
    //After updating (if so), fetch the remaining
}

CVt_pushCallback CVt_pushCallbackPtr = &CVt_pushHandle;
