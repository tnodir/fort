#ifndef DBUTIL_H
#define DBUTIL_H

#include <QObject>

class SqliteDb;

class DbUtil
{
public:
    static int getFreeId(SqliteDb *sqliteDb, const char *sqlSelectIds, int maxCount, bool &ok);
};

#endif // DBUTIL_H
