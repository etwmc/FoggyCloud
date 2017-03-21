//
//  CVtCloudTranscation.h
//  CloudVault
//
//  Created by Wai Man Chan on 2/6/17.
//
//

#ifndef CVtCloudTranscation_h
#define CVtCloudTranscation_h

#include <stdio.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <CVtKeychain.h>

#define opNameToPtr(A) A ## _Pointer
#define opNameToOnceMacro(A) A ## _Once

#define defineOperation(opName, rollbackName) \
static operationPointer opNameToPtr(opName) = registerOperation(opName, rollbackName);
#define newProtocolVersion() _newProtocolVersion();

typedef enum {
    //When the transcation line up on the backend, and all previous transcation has carried out
    CVtOperationStatus_Success = 0,
    //When the transcation can not passed to the backend, but flush into the running log
    CVtOperationStatus_Pending,
    //When the transcation reached the backend, but the requirement to carry out does not mean the requirement
    //e.g. When trying to change the color of a section of text from black to red, but text was switch from black to white already.
    //It requires either the application involvement (possibly based on user interaction) to select whether the should the transcation run through discarding the failed condition
    //The determiniation is occured at the callback function, by either pass through or rollback
    CVtOperationStatus_Conflict,
    //When a operation is repeat a previous event, which can detect using the existing status
    //e.g. chagne the text to the same color twice
    CVtOperationStatus_Repeated,
    //When a remote transcation has reached the receiving end. It only trigger on remote transcation
    CVtOperationStatus_Delivered,
} CVtOperationStatus;

typedef struct {
    char data[16];
} UUID;
char *uuidToStr(UUID uuid);
typedef enum {
    CVtOperationCallbackResponse_PassThr,
    CVtOperationCallbackResponse_Rollback,
} CVtOperationCallbackResponse;
typedef CVtOperationCallbackResponse (*CVtOperationCallback)(UUID transcationUUID, CVtOperationStatus status);
//When ignorePrecondition is false, the operation should check the condition before 
typedef CVtOperationStatus (*CVtOperationPrototype)(void *arguments, size_t argumentSize, bool ignorePrecondition);

typedef struct {
    unsigned int protocolVersion;
    unsigned int opIndex;
} CVtOperationPointer;

CVtOperationPointer registerOperation(CVtOperationPrototype operation, CVtOperationPrototype rollback);
void _newProtocolVersion();

UUID issueOperation(CVtOperationPointer operationPtr, void *args, size_t argsLen, CVtOperationCallback callback);
UUID issueRemoteOperation(userID targetUser, CVtOperationPointer operationPtr, void *args, size_t argsLen, CVtOperationCallback callback);

#endif /* CVtCloudTranscation_h */
