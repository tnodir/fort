#ifndef DATABASESQL_H
#define DATABASESQL_H

class DatabaseSql
{
public:
    static const char * const sqlPragmas;
    static const char * const sqlCreateTables;

    static const char * const sqlSelectAppId;
    static const char * const sqlInsertAppId;

    static const char * const sqlSelectAppPaths;

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

    static const char * const sqlSelectMinTrafAppHour;
    static const char * const sqlSelectMinTrafAppDay;
    static const char * const sqlSelectMinTrafAppMonth;
    static const char * const sqlSelectMinTrafAppTotal;

    static const char * const sqlSelectMinTrafHour;
    static const char * const sqlSelectMinTrafDay;
    static const char * const sqlSelectMinTrafMonth;
    static const char * const sqlSelectMinTrafTotal;

    static const char * const sqlSelectTrafAppHour;
    static const char * const sqlSelectTrafAppDay;
    static const char * const sqlSelectTrafAppMonth;
    static const char * const sqlSelectTrafAppTotal;

    static const char * const sqlSelectTrafHour;
    static const char * const sqlSelectTrafDay;
    static const char * const sqlSelectTrafMonth;
    static const char * const sqlSelectTrafTotal;
};

#endif // DATABASESQL_H
