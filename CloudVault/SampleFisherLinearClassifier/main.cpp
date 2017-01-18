//
//  main.c
//  SampleFisherLinearClassifier
//
//  Created by Wai Man Chan on 1/9/17.
//
//

#include <stdio.h>

#include <CloudVault.h>

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    for(i=0; i<argc; i++){
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

int main(int argc, const char * argv[]) {
    // insert code here...
    
    sqlite3 *db;
    int rc = 0;
    char *sql;
    char *zErrMsg = 0;
    
    encryptionSqliteInit();
    
    rc |= encryptSqlite3_open("sampleData.db", &db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return(0);
    }else{
        fprintf(stdout, "Opened database successfully\n");
    }
    
    //Create database
    sql = "CREATE TABLE image_3x3 ("\
    "ID INTEGER PRIMARY KEY   AUTOINCREMENT,"\
    
    "val0x0 REAL,"\
    "val0x1 REAL,"\
    "val0x2 REAL,"\
    
    "val1x0 REAL,"\
    "val1x1 REAL,"\
    "val1x2 REAL,"\
    
    "val2x0 REAL,"\
    "val2x1 REAL,"\
    "val2x2 REAL);";
    
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "INSERT SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Records created successfully\n");
    }
    
    for (int i = 0; i < 100; i++) {
        double vector[9];
        for (int j = 0; j <9; j++) {
            vector[j] = 
        }
    }
    
    sqlite3_close(db);
    
    return 0;
}
