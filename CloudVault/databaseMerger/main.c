//
//  main.c
//  databaseMerger
//
//  Created by Wai Man Chan on 1/8/17.
//
//

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/mman.h>

#include "CVtKeyManagment.h"
#include "EncryptionBlocker.h"

int symmetryKeySize = 32;
int asymmetryKeySize = 512;

int main(int argc, const char * argv[]) {
    //File address
    const char *addr = argv[1];
    
    dbMerger(addr);
    
    return 0;
}

