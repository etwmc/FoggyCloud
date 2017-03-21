//
//  CVtEventualMerger.h
//  CloudVault
//
//  Created by Wai Man Chan on 3/6/17.
//
//

#ifndef CVtEventualMerger_h
#define CVtEventualMerger_h

#include <stdio.h>

typedef struct CVt_RemoteUUIDString {
    char str[37];
} CVt_RemoteUUIDString;

//The push payload from the server being JSON string, ownership by the callback (i.e. dealloc there)
typedef void (*CVt_pushCallback)(void *message);
extern CVt_pushCallback CVt_pushCallbackPtr;

#endif /* CVtEventualMerger_h */
