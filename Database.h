#pragma once

#include "utils/CSingleton.h"

#include "Logger.h"

#include <inttypes.h>
#include <map>
#include <string>


#include <sqlite/sqlite3.h>

class Measurement;

class Database : public CSingleton < Database >
{
    friend class CSingleton < Database >;

public:
    Database() = default;
    void Init(void);
    void InsertMeasurement(std::shared_ptr<Measurement>& m);

private:
    void SendQuery(std::string&& query, void(Database::*execute_function)(sqlite3_stmt* stmt));
    

    bool ExecuteQuery(char* query, int (*callback)(void*, int, char**, char**) = NULL);
    bool Open(void);
    int Callback(void* NotUsed, int argc, char** argv, char** azColName);

    void Query_Latest(sqlite3_stmt* stmt);
    void Query_1Day(sqlite3_stmt* stmt);
    void Query_1Week(sqlite3_stmt* stmt);
    sqlite3* db;
    int rc;
    char* sql;
};