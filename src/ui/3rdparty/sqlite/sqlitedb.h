#ifndef SQLITEDB_H
#define SQLITEDB_H

#include <QString>
#include <QVariant>

#include "../../util/classhelpers.h"

QT_FORWARD_DECLARE_STRUCT(sqlite3)

QT_FORWARD_DECLARE_CLASS(SqliteDb)
QT_FORWARD_DECLARE_CLASS(SqliteStmt)

using SQLITEDB_MIGRATE_FUNC = bool (*)(SqliteDb *db, int version, void *context);

class SqliteDb
{
public:
    explicit SqliteDb(const QString &filePath = QString());
    ~SqliteDb();
    CLASS_DEFAULT_COPY_MOVE(SqliteDb)

    struct sqlite3 *db() const { return m_db; }

    QString filePath() const { return m_filePath; }

    bool open(const QString &filePath = QString());
    void close();

    bool attach(const QString &schemaName,
                const QString &filePath = QString());
    bool detach(const QString &schemaName);

    bool execute(const char *sql);
    bool execute16(const ushort *sql);
    bool executeStr(const QString &sql);

    QVariant executeEx(const char *sql,
                       const QVariantList &vars = QVariantList(),
                       int resultCount = 1,
                       bool *ok = nullptr);

    bool prepare(SqliteStmt &stmt, const char *sql,
                 const QVariantList &vars = QVariantList());

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

    static QString entityName(const QString &schemaName,
                              const QString &objectName);
    QStringList tableNames(const QString &schemaName = QString());
    QStringList columnNames(const QString &tableName,
                            const QString &schemaName = QString());

    bool migrate(const QString &sqlDir, int version,
                 bool recreate = false,
                 bool importOldData = false,
                 SQLITEDB_MIGRATE_FUNC migrateFunc = nullptr,
                 void *migrateContext = nullptr);

    bool importDb(const QString &sourceFilePath);

private:
    sqlite3 *m_db;
    QString m_filePath;
};

#endif // SQLITEDB_H
