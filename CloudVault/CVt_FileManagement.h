//
//  CVt_FileManagement.h
//  CloudVault
//
//  Created by Wai Man Chan on 2/14/17.
//
//

#ifndef CVt_FileManagement_h
#define CVt_FileManagement_h

#include <stdio.h>
#include <stdbool.h>

size_t CVt_rawReadback(const char **buffer, const char *addressPrefix, const char *address);
size_t CVt_offsetReadback(const char **buffer, size_t offset, const char *addressPrefix, const char *address);
size_t CVt_partialReadback(const char **buffer, size_t size, const char *addressPrefix, const char *address);
void CVt_rawFlushDown(const char *buffer, size_t len, const char *addressPrefix, const char *address);

//If the file exist
bool CVt_FileManagmentRequestCanFulfill(const char *addressPrefix, const char *address);

#endif /* CVt_FileManagement_h */
