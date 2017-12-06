#ifndef SQLITESTMT_H
#define SQLITESTMT_H

#include <QString>
#include <QVariant>

QT_FORWARD_DECLARE_STRUCT(sqlite3_stmt)

class SqliteStmt
{
public:
    enum PrepareFlags {
        PrepareDefault      = 0x00,
        PreparePersistent   = 0x01  // SQLITE_PREPARE_PERSISTENT
    };

    enum StepResult {
        StepError   = -1,
        StepRow     = 100,  // SQLITE_ROW
        StepDone    = 101   // SQLITE_DONE
    };

    explicit SqliteStmt();
    ~SqliteStmt();

    bool prepare(struct sqlite3 *db, const char *sql,
                 PrepareFlags flags = PrepareDefault);
    void finalize();

    bool bindInt(int index, qint32 number);
    bool bindInt64(int index, qint64 number);
    bool bindNull(int index = 1);
    bool bindText(int index, const QString &text);

    bool clearBindings();
    bool reset();

    bool isBusy();

    StepResult step();

    int dataCount();

    qint32 columnInt(int column = 0);
    qint64 columnInt64(int column = 0);
    QString columnText(int column = 0);

private:
    sqlite3_stmt *m_stmt;

    QHash<int, QVariant> m_bindObjects;
};

#endif // SQLITESTMT_H
