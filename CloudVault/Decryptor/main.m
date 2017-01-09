//
//  main.c
//  Decryptor
//
//  Created by Wai Man Chan on 12/31/16.
//
//

#include <stdio.h>
#include <CommonCrypto/CommonCrypto.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>
#include <sys/mman.h>
#include <CloudVault.h>

int main(int argc, const char * argv[]) {
    // insert code here...
    NSString *addr = [@"./test.db.store" stringByExpandingTildeInPath];
    NSData *data = [NSData dataWithContentsOfFile:addr];
    char key[32];
    copyMasterKey_self("./test.db", key);
    char zero[16];
    bzero(zero, 16);
    char *output = malloc(sizeof(char)*data.length);
    lowLevelDecrypt(key, 32, zero, 16, data.bytes, data.length, output);
    NSData *dec = [NSData dataWithBytesNoCopy:output length:data.length];
    [dec writeToFile:[addr stringByAppendingPathExtension:@"expand"] atomically:true];
    return 0;
}