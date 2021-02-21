#ifndef STATSQL_H
#define STATSQL_H

class StatSql
{
public:
    static const char *const sqlSelectAppId;
    static const char *const sqlInsertAppId;

    static const char *const sqlSelectDeletedStatAppPaths;
    static const char *const sqlDeleteStatAppId;

    static const char *const sqlSelectStatAppPaths;

    static const char *const sqlInsertTrafAppHour;
    static const char *const sqlInsertTrafAppDay;
    static const char *const sqlInsertTrafAppMonth;
    static const char *const sqlInsertTrafAppTotal;

    static const char *const sqlInsertTrafHour;
    static const char *const sqlInsertTrafDay;
    static const char *const sqlInsertTrafMonth;

    static const char *const sqlUpdateTrafAppHour;
    static const char *const sqlUpdateTrafAppDay;
    static const char *const sqlUpdateTrafAppMonth;

    static const char *const sqlUpdateTrafHour;
    static const char *const sqlUpdateTrafDay;
    static const char *const sqlUpdateTrafMonth;

    static const char *const sqlUpdateTrafAppTotal;

    static const char *const sqlSelectMinTrafAppHour;
    static const char *const sqlSelectMinTrafAppDay;
    static const char *const sqlSelectMinTrafAppMonth;
    static const char *const sqlSelectMinTrafAppTotal;

    static const char *const sqlSelectMinTrafHour;
    static const char *const sqlSelectMinTrafDay;
    static const char *const sqlSelectMinTrafMonth;
    static const char *const sqlSelectMinTrafTotal;

    static const char *const sqlSelectTrafAppHour;
    static const char *const sqlSelectTrafAppDay;
    static const char *const sqlSelectTrafAppMonth;
    static const char *const sqlSelectTrafAppTotal;

    static const char *const sqlSelectTrafHour;
    static const char *const sqlSelectTrafDay;
    static const char *const sqlSelectTrafMonth;
    static const char *const sqlSelectTrafTotal;

    static const char *const sqlDeleteTrafAppHour;
    static const char *const sqlDeleteTrafAppDay;
    static const char *const sqlDeleteTrafAppMonth;

    static const char *const sqlDeleteTrafHour;
    static const char *const sqlDeleteTrafDay;
    static const char *const sqlDeleteTrafMonth;

    static const char *const sqlDeleteAppTrafHour;
    static const char *const sqlDeleteAppTrafDay;
    static const char *const sqlDeleteAppTrafMonth;
    static const char *const sqlDeleteAppTrafTotal;

    static const char *const sqlResetAppTrafTotals;

    static const char *const sqlInsertConn;
    static const char *const sqlInsertConnBlock;

    static const char *const sqlSelectOldConnBlock;

    static const char *const sqlSelectDeletedRangeConnAppPaths;
    static const char *const sqlDeleteRangeConnAppId;
    static const char *const sqlDeleteRangeConn;
    static const char *const sqlDeleteRangeConnBlock;

    static const char *const sqlSelectDeletedAllConnAppPaths;
    static const char *const sqlDeleteAllConnAppId;
    static const char *const sqlDeleteAllConn;
    static const char *const sqlDeleteAllConnBlock;
};

#endif // STATSQL_H
