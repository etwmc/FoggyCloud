//
//  CVtPlatformCode_NetworkDarwinHelper.h
//  CloudVault
//
//  Created by Wai Man Chan on 2/28/17.
//
//

#import "CVtPlatformCode_Network.h"

void *addNewRequest(CVtNetwork_httpRequest request, networkCallback callback, callbackArg arg);
void cancelRequest(void*requestToken);
