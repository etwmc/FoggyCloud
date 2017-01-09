//
//  main.c
//  PartialDecryptTest
//
//  Created by Wai Man Chan on 12/29/16.
//
//

#include <stdio.h>
#include <CloudVault.h>
#include <string.h>

#include <assert.h>

int main(int argc, const char * argv[]) {
    // insert code here...
    
    //Generate a full encryption
    char zeros[8192];
    bzero(zeros, 8192);
    
    //Generate Key
    char key[32];
    generateSymmetricKey(key, 32);
    //Generate IV
    char iv[32];
    bzero(iv, 32);
    
    //Fully encryption
    char ciphertext[8192];
    lowLevelEncrypt(key, 32, iv, 16, zeros, 8192, ciphertext);
    char decrypted[8192];
    //Decode the first 16 bytes
    lowLevelDecrypt(key, 32, iv, 16, ciphertext, 16, decrypted);
    //Decode the second set of bytes using the first 16 as an IV
    lowLevelDecrypt(key, 32, ciphertext, 16, ciphertext+16, 8192-16, decrypted+16);
    
    //Compare
    assert( bcmp(zeros, decrypted, 8192) == 0 );
    
    return 0;
}
