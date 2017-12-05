#ifndef DATABASESQL_H
#define DATABASESQL_H

class DatabaseSql
{
public:
    static const char * const sqlPragmas;
    static const char * const sqlCreateTables;

    static const char * const sqlSelectAppId;
    static const char * const sqlInsertAppId;

    static const char * const sqlInsertTrafficAppHour;
    static const char * const sqlInsertTrafficAppDay;
    static const char * const sqlInsertTrafficAppMonth;

    static const char * const sqlInsertTrafficHour;
    static const char * const sqlInsertTrafficDay;
    static const char * const sqlInsertTrafficMonth;

    static const char * const sqlUpdateTrafficAppHour;
    static const char * const sqlUpdateTrafficAppDay;
    static const char * const sqlUpdateTrafficAppMonth;

    static const char * const sqlUpdateTrafficHour;
    static const char * const sqlUpdateTrafficDay;
    static const char * const sqlUpdateTrafficMonth;

    static const char * const sqlUpdateTrafficAppTotal;
};

#endif // DATABASESQL_H
