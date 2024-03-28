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

    QString expandedSql() const;

    int bindParameterIndex(const QString &name) const;

    bool bindInt(int index, qint32 number);
    bool bindInt64(int index, qint64 number);
    bool bindDouble(int index, double number);
    bool bindNull(int index = 1);
    bool bindText(int index, const QString &text);
    bool bindDateTime(int index, const QDateTime &dateTime);
    bool bindBlob(int index, const QByteArray &data);
    bool bindDataStream(int index, const QVariant &v);
    bool bindVar(int index, const QVariant &v);
    bool bindVars(const QVariantList &vars, int index = 1);
    bool bindVarsMap(const QVariantHash &varsMap);

    bool clearBindings();
    bool reset();

    bool isBusy() const;

    SqliteStmt::StepResult step();

    int dataCount() const;
    int columnCount() const;

    QString columnName(int column = 0) const;
    qint32 columnInt(int column = 0) const;
    quint32 columnUInt(int column = 0) const;
    qint64 columnInt64(int column = 0) const;
    quint64 columnUInt64(int column = 0) const;
    double columnDouble(int column = 0) const;
    bool columnBool(int column = 0) const;
    QString columnText(int column = 0) const;
    QDateTime columnDateTime(int column = 0) const;
    QDateTime columnUnixTime(int column = 0) const;
    QByteArray columnBlob(int column = 0, bool isRaw = false) const;
    QVariant columnDataStream(int column = 0) const;
    QVariant columnVar(int column = 0) const;
    bool columnIsNull(int column = 0) const;

private:
    sqlite3_stmt *m_stmt = nullptr;

    QHash<int, QVariant> m_bindObjects;
};

#endif // SQLITESTMT_H
