//
//  CVt_LocalJournal.h
//  CloudVault
//
//  Created by Wai Man Chan on 2/14/17.
//
//

#ifndef CVt_LocalJournal_h
#define CVt_LocalJournal_h

#include <stdio.h>

void writeToLocalJournal(const char *data, size_t dataLen);
void readFromLocalJournal(const char **data, size_t *dataLen, size_t *numberOfData);

void writeSecure_ToLocalJournal(const char *data, size_t dataLen, const char *key);
void readSecure_FromLocalJournal(const char **data, size_t *dataLen, size_t *numberOfData, const char *key);

#endif /* CVt_LocalJournal_h */
