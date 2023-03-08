#ifndef SQLITETYPES_H
#define SQLITETYPES_H

#include <QList>
#include <QSharedPointer>

class SqliteDb;
class SqliteStmt;

using SqliteDbPtr = QSharedPointer<SqliteDb>;

using SqliteStmtList = QList<SqliteStmt *>;

#endif // SQLITETYPES_H
