#ifndef SQLITEDB_H
#define SQLITEDB_H

#include <QString>

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

    qint64 lastInsertRowid();

    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    QString errorMessage() const;

private:
    sqlite3 *m_db;
};

#endif // SQLITEDB_H
