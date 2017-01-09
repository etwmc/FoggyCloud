//
//  DeltaPager.h
//  
//
//  Created by Wai Man Chan on 12/1/16.
//
//

#ifndef DeltaPager_h
#define DeltaPager_h

#include <stdio.h>
#include <sqlite3.h>

void encryptionSqliteInit();

int encryptSqlite3_open(const char *filename, sqlite3 **ppDb);
int encryptSqlite3_open_v2(const char *filename, sqlite3 **ppDb, int flags, const char *zVfs);

#endif /* DeltaPager_h */
