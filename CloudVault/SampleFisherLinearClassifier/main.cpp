//
//  main.c
//  SampleFisherLinearClassifier
//
//  Created by Wai Man Chan on 1/9/17.
//
//

#include <stdio.h>
#include <random>

extern "C" {
    #include <CloudVault.h>
}

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
    
    rc |= encryptSqlite3_open("sampleData.ec.db", &db);
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
    "val2x2 REAL,"\
    
    "type INTEGER);";
    
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "DB SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "DB created successfully\n");
    }
    
    /*sql = "PRAGMA schema.wal_checkpoint;";
    //rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "DB SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "DB created successfully\n");
    }*/
    
    const double sd = 0.0;
    
    std::default_random_engine _randomGenerator;
    std::random_device rd;
    _randomGenerator.seed(rd());
    std::normal_distribution<double> _distribution(0, 10);
    
    //Generate center
    double VectorA[9];
    double VectorB[9];
    for (int i = 0; i < 9; i++) {
        VectorA[i] = _distribution(_randomGenerator);
    }
    for (int i = 0; i < 9; i++) {
        VectorB[i] = _distribution(_randomGenerator);
    }
    
    srand(time(NULL));
    
    _distribution = std::normal_distribution<double>(0, 0.1);
    char *insertSQLFormat = "INSERT INTO image_3x3 "  \
    "VALUES ( NULL, %f, %f, %f, %f, %f, %f, %f, %f, %f, %d ); ";
    for (int i = 0; i < 1000000; i++) {
        double vector[9];
        bool useVersionA = ((double)rand())/RAND_MAX > 0.5;
        double *chosenVector = useVersionA? VectorA: VectorB;
        for (int j = 0; j <9; j++) {
            double noise = _distribution(_randomGenerator);
            vector[j] = chosenVector[j]+noise;
        }
        char insertSQL[1024];
        snprintf(insertSQL, 1024, insertSQLFormat, vector[0], vector[1], vector[2]
                 , vector[3], vector[4], vector[5]
                 , vector[6], vector[7], vector[8], (int)useVersionA );
        rc = sqlite3_exec(db, insertSQL, callback, 0, &zErrMsg);
        if( rc != SQLITE_OK ){
            fprintf(stderr, "INSERT SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }else{
            //fprintf(stdout, "Records INSERT successfully\n");
        }
    }
    
    char *deleteAllData = "DELETE FROM image_3x3;";
    rc = sqlite3_exec(db, deleteAllData, callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "DELETE SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Records DELETE successfully\n");
    }
    
    sqlite3_close(db);
    
    return 0;
}
