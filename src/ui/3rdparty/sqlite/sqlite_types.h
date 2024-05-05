#ifndef SQLITE_TYPES_H
#define SQLITE_TYPES_H

#include <QList>
#include <QSharedPointer>

class SqliteDb;
class SqliteStmt;

using SqliteDbPtr = QSharedPointer<SqliteDb>;

using SqliteStmtList = QList<SqliteStmt *>;

#endif // SQLITE_TYPES_H
