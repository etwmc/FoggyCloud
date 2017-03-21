//
//  CVtProjectConstant.h
//  CloudVault
//
//  Created by Wai Man Chan on 2/7/17.
//
//

#ifndef CVtProjectConstant_h
#define CVtProjectConstant_h

//DB Operation Parameter
//Default round trip time to consider good network: 500ms
//The variable will adjust using moving average+2 s.d., but 500ms will be the default moving average, where 0ms is the s.d.
#define CVtProjectConstant_guidanceRoundTripTime 0.5

//DB backend constant

#define CVtProjectConstant_dbBackendAddr "localhost"
#define CVtProjectConstant_dbBackendPort "8080"
#define CVtProjectConstant_dbHTTPProtocol "https"

#define CVtProjectConstant_dbUserKeyDir "/key"
#define CVtProjectConstant_dbMsgJourDir "/message"
#define CVtProjectConstant_dbQueueDir   "/queue"
#define CVtProjectConstant_dbMsgContDir "/msg"
#define CVtProjectConstant_dbSnapshotDir "/snapshot"
#define CVtProjectConstant_dbInitPortal "/init"

#define CVt_dbBackendURL(dir) CVtProjectConstant_dbHTTPProtocol "://" CVtProjectConstant_dbBackendAddr ":" CVtProjectConstant_dbBackendPort dir

//File Managment
#define CVtProjectConstant_fileRoot     "./"
#define CVtProjectConstant_fileRootFdName ""
#define CVtProjectConstant_fileAssetFdName "Asset/"
#define CVtProjectConstant_fileCacheKeyFdName "CacheKey/"

#define CVt_fileAddr(fdName) CVtProjectConstant_fileRoot fdName

#define CVt_stringMerge(strA, strB) strA strB

#endif /* CVtProjectConstant_h */
