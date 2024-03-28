#ifndef DBUTIL_H
#define DBUTIL_H

#include <QObject>

#include "sqlitetypes.h"

class DbUtil
{
public:
    static void doList(const SqliteStmtList &stmtList);
};

#endif // DBUTIL_H
