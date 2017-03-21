//
//  CVtUserManagment.c
//  CloudVault
//
//  Created by Wai Man Chan on 3/18/17.
//
//

#include "CVtUserManagment.h"
#include <stdlib.h>
#include <string.h>
#include "CVtKeyManagment.h"
#include "CVtProjectConstant.h"

char userID[37] = {0};

const char *readableUserID() {
    char *result = malloc(37);
    strncpy(result, userID, 37);
    return result;
}

void setUserID_readable(const char *uuid) {
    strncpy(userID, uuid, 37);
}

void createNewUser() {
    //Generate keys
    generateKeyBag(CVt_stringMerge(CVt_fileAddr(CVtProjectConstant_fileCacheKeyFdName), "tempKey"));
}
