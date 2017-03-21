//
//  CVtPlatformCode_PushNotification.c
//  CloudVault
//
//  Created by Wai Man Chan on 3/6/17.
//
//

#include "CVtPlatformCode_PushNotification.h"

//The push payload from the server being JSON string, ownership by the callback (i.e. dealloc there)
typedef void (*CVt_pushCallback)(void *message);
extern CVt_pushCallback CVt_pushCallbackPtr;

//TODO: Implement the push receiver. When find a push, use the callback to pass the payload.
