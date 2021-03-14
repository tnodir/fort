#ifndef SQLITEDB_H
#define SQLITEDB_H

#include <QString>
#include <QVariant>

#include "../../util/classhelpers.h"

struct sqlite3;

class SqliteDb;
class SqliteStmt;

using SQLITEDB_MIGRATE_FUNC = bool (*)(SqliteDb *db, int version, bool isNewDb, void *context);

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

    bool attach(const QString &schemaName, const QString &filePath = QString());
    bool detach(const QString &schemaName);

    bool execute(const char *sql);
    bool executeStr(const QString &sql);

    QVariant executeEx(const char *sql, const QVariantList &vars = QVariantList(),
            int resultCount = 1, bool *ok = nullptr);

    bool prepare(SqliteStmt &stmt, const char *sql, const QVariantList &vars = QVariantList());
    bool done(SqliteStmt *stmt);

    qint64 lastInsertRowid() const;
    int changes() const;

    bool beginTransaction();
    bool endTransaction(bool ok = true);
    bool commitTransaction();
    bool rollbackTransaction();

    bool beginSavepoint(const char *name = nullptr);
    bool releaseSavepoint(const char *name = nullptr);
    bool rollbackSavepoint(const char *name = nullptr);

    int errorCode() const;
    QString errorMessage() const;

    int userVersion();
    bool setUserVersion(int v);

    QString encoding();
    bool setEncoding(const QString &v);

    static QString migrationOldSchemaName();
    static QString migrationNewSchemaName();
    static QString entityName(const QString &schemaName, const QString &objectName);
    QStringList tableNames(const QString &schemaName = QString());
    QStringList columnNames(const QString &tableName, const QString &schemaName = QString());

    bool migrate(const QString &sqlDir, const char *sqlPragmas, int version, bool recreate = false,
            bool importOldData = false, SQLITEDB_MIGRATE_FUNC migrateFunc = nullptr,
            void *migrateContext = nullptr);

    bool importDb(
            const QString &sourceFilePath, SQLITEDB_MIGRATE_FUNC migrateFunc, void *migrateContext);

private:
    sqlite3 *m_db = nullptr;
    QString m_filePath;
};

#endif // SQLITEDB_H
