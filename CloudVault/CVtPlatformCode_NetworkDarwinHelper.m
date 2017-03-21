//
//  CVtPlatformCode_NetworkDarwinHelper.m
//  CloudVault
//
//  Created by Wai Man Chan on 2/28/17.
//
//

#import <Foundation/Foundation.h>
#import "CVtPlatformCode_NetworkDarwinHelper.h"

@interface CVtPlatformCode_NetworkDarwinHelper : NSObject {
    NSURLSession *session;
    NSURLSessionTask *currentTask;
}
+ (instancetype)shareHelper;
- (__weak NSObject*)addRequest:(NSURLRequest *)request callback:(void(^)(NSURLRequest*, NSURLResponse*, NSData*))callback;
- (void)cancelRequest:(__weak NSObject*)request;
@end


@implementation CVtPlatformCode_NetworkDarwinHelper
+ (instancetype)shareHelper {
    static CVtPlatformCode_NetworkDarwinHelper *helper = NULL;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        helper = [CVtPlatformCode_NetworkDarwinHelper new];
    });
    return helper;
}
- (id)init {
    self = [super init];
    if (self) {
        session = [NSURLSession sharedSession];
    }
    return self;
}
- (__weak NSObject*)addRequest:(NSURLRequest *)request callback:(void(^)(NSURLRequest*, NSURLResponse*, NSData*))callback {
    currentTask = [session dataTaskWithRequest:request completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
        if (error) {
            //Retry
            [self addRequest:request callback:callback];
        } else {
            callback(request, response, data);
        }
    }];
    [currentTask resume];
    return currentTask;
}
- (void)cancelRequest:(__weak NSObject*)request {
    NSURLSessionTask *task = request;
    [task cancel];
}
@end


void *addNewRequest(CVtNetwork_httpRequest request, networkCallback callback, callbackArg arg) {
    NSMutableURLRequest *_request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:[NSString stringWithUTF8String:request.path]]];
    [_request setHTTPMethod:[NSString stringWithUTF8String:methodToStr(request.method)]];
    for (int i = 0; i < request.headerSize; i++) {
        const char *field = request.header[i].header;
        const char *value = request.header[i].value;
        [_request setValue:[NSString stringWithUTF8String:value] forHTTPHeaderField:[NSString stringWithUTF8String:field]];
    }
    [_request setHTTPBody:[NSData dataWithBytes:request.body length:request.bodySize]];
    //Clean up request
    CVtNetworkStack_requestRelease(request);
    //Formed the request, so send
    return (__bridge void *)([[CVtPlatformCode_NetworkDarwinHelper shareHelper] addRequest:_request callback:^(NSURLRequest *req, NSURLResponse *res, NSData *data) {
        if ([res isKindOfClass:[NSHTTPURLResponse class]]) {
            CVtNetwork_httpResponse _res;
            _res.statusCode = ((NSHTTPURLResponse*)res).statusCode;
            _res.body = malloc(_res.bodyLen);
            bcopy(data.bytes, (char*)_res.body, _res.bodyLen);
            callback(arg,_res);
        }
        
    }]);
}

void cancelRequest(void*requestToken) {
    [[CVtPlatformCode_NetworkDarwinHelper shareHelper] cancelRequest:(__bridge NSObject *)(requestToken)];
}
