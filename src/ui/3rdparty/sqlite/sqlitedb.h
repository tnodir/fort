#ifndef SQLITEDB_H
#define SQLITEDB_H

#include <QString>
#include <QVariant>

#include "../../util/classhelpers.h"

QT_FORWARD_DECLARE_CLASS(SqliteDb)
QT_FORWARD_DECLARE_STRUCT(sqlite3)

using SQLITEDB_MIGRATE_FUNC = bool (*)(SqliteDb *db, int version, void *context);

class SqliteDb
{
public:
    explicit SqliteDb();
    ~SqliteDb();
    CLASS_DEFAULT_COPY_MOVE(SqliteDb)

    bool open(const QString &filePath);
    void close();

    struct sqlite3 *db() const { return m_db; }

    bool execute(const char *sql);
    bool execute16(const ushort *sql);
    bool executeStr(const QString &sql);

    QVariant executeEx(const char *sql,
                       const QVariantList &vars = QVariantList(),
                       int resultCount = 1,
                       bool *ok = nullptr);

    qint64 lastInsertRowid() const;
    int changes() const;

    bool beginTransaction();
    bool endTransaction(bool ok = true);
    bool commitTransaction();
    bool rollbackTransaction();

    bool beginSavepoint(const char *name = nullptr);
    bool releaseSavepoint(const char *name = nullptr);
    bool rollbackSavepoint(const char *name = nullptr);

    QString errorMessage() const;

    int userVersion();

    bool migrate(const QString &sqlDir, int version,
                 SQLITEDB_MIGRATE_FUNC migrateFunc = nullptr,
                 void *migrateContext = nullptr);

private:
    sqlite3 *m_db;
};

#endif // SQLITEDB_H
