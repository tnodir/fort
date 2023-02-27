#ifndef SQLITESTMT_H
#define SQLITESTMT_H

#include <QDateTime>
#include <QString>
#include <QVariant>

#include <util/classhelpers.h>

#include "sqlitetypes.h"

struct sqlite3_stmt;

class SqliteStmt
{
public:
    enum PrepareFlags {
        PrepareDefault = 0x00,
        PreparePersistent = 0x01 // SQLITE_PREPARE_PERSISTENT
    };

    enum StepResult {
        StepError = -1,
        StepRow = 100, // SQLITE_ROW
        StepDone = 101 // SQLITE_DONE
    };

    explicit SqliteStmt();
    ~SqliteStmt();
    CLASS_DEFAULT_COPY_MOVE(SqliteStmt)

    bool prepare(struct sqlite3 *db, const char *sql, PrepareFlags flags = PrepareDefault);
    void finalize();

    QString expandedSql();

    bool bindInt(int index, qint32 number);
    bool bindInt64(int index, qint64 number);
    bool bindDouble(int index, double number);
    bool bindNull(int index = 1);
    bool bindText(int index, const QString &text);
    bool bindDateTime(int index, const QDateTime &dateTime);
    bool bindBlob(int index, const QByteArray &data);
    bool bindVarBlob(int index, const QVariant &v);
    bool bindVar(int index, const QVariant &v);
    bool bindVars(const QVariantList &vars, int index = 1);

    bool clearBindings();
    bool reset();

    bool isBusy();

    SqliteStmt::StepResult step();

    int dataCount();
    int columnCount();

    QString columnName(int column = 0);
    qint32 columnInt(int column = 0);
    qint64 columnInt64(int column = 0);
    double columnDouble(int column = 0);
    bool columnBool(int column = 0);
    QString columnText(int column = 0);
    QDateTime columnDateTime(int column = 0);
    QDateTime columnUnixTime(int column = 0);
    QByteArray columnBlob(int column = 0, bool isRaw = false);
    QVariant columnVarBlob(int column = 0);
    QVariant columnVar(int column = 0);
    bool columnIsNull(int column = 0);

    static void doList(const SqliteStmtList &stmtList);

private:
    sqlite3_stmt *m_stmt = nullptr;

    QHash<int, QVariant> m_bindObjects;
};

#endif // SQLITESTMT_H
