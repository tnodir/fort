#ifndef MOCKSQLITESTMT_H
#define MOCKSQLITESTMT_H

#include <googletest.h>

#include <sqlite/sqlitestmt.h>

class MockSqliteStmt : public SqliteStmt
{
public:
    explicit MockSqliteStmt();

    MOCK_METHOD(SqliteStmt::StepResult, step, ());

    MOCK_METHOD(qint32, columnInt, (int column), (const));
};

#endif // MOCKSQLITESTMT_H
