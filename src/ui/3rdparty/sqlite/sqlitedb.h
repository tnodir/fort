#ifndef SQLITEDB_H
#define SQLITEDB_H

#include <QString>
#include <QVariant>

QT_FORWARD_DECLARE_STRUCT(sqlite3)

class SqliteDb
{
public:
    explicit SqliteDb();
    ~SqliteDb();

    bool open(const QString &filePath);
    void close();

    struct sqlite3 *db() const { return m_db; }

    bool execute(const char *sql);
    bool execute16(const ushort *sql);
    bool executeStr(const QString &sql);

    QVariant executeOut(const char *sql);

    qint64 lastInsertRowid() const;
    int changes() const;

    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    bool beginSavepoint(const char *name = nullptr);
    bool releaseSavepoint(const char *name = nullptr);
    bool rollbackSavepoint(const char *name = nullptr);

    QString errorMessage() const;

    bool migrate(const QString &sqlDir, int version);

private:
    sqlite3 *m_db;
};

#endif // SQLITEDB_H
