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
    enum OpenFlag {
        OpenReadOnly = 0x00000001, // SQLITE_OPEN_READONLY
        OpenReadWrite = 0x00000002, // SQLITE_OPEN_READWRITE
        OpenCreate = 0x00000004, // SQLITE_OPEN_CREATE
        OpenUri = 0x00000040, // SQLITE_OPEN_URI
        OpenMemory = 0x00000080, // SQLITE_OPEN_MEMORY
        OpenNoMutex = 0x00008000, // SQLITE_OPEN_NOMUTEX
        OpenFullMutex = 0x00010000, // SQLITE_OPEN_FULLMUTEX
        OpenSharedCache = 0x00020000, // SQLITE_OPEN_SHAREDCACHE
        OpenPrivateCache = 0x00040000, // SQLITE_OPEN_PRIVATECACHE
        OpenNoFollow = 0x01000000, // SQLITE_OPEN_NOFOLLOW
        OpenDefaultReadOnly = (OpenReadOnly | OpenNoMutex),
        OpenDefaultReadWrite = (OpenReadWrite | OpenCreate | OpenNoMutex)
    };

    explicit SqliteDb(
            const QString &filePath = QString(), quint32 openFlags = OpenDefaultReadWrite);
    ~SqliteDb();
    CLASS_DEFAULT_COPY_MOVE(SqliteDb)

    struct sqlite3 *db() const { return m_db; }

    quint32 openFlags() const { return m_openFlags; }
    void setOpenFlags(quint32 v) { m_openFlags = v; }

    QString filePath() const { return m_filePath; }
    void setFilePath(const QString &v) { m_filePath = v; }

    bool open();
    void close();

    bool attach(const QString &schemaName, const QString &filePath = QString());
    bool detach(const QString &schemaName);

    bool execute(const char *sql);
    bool executeStr(const QString &sql);

    QVariant executeEx(const char *sql, const QVariantList &vars = QVariantList(),
            int resultCount = 1, bool *ok = nullptr);

    bool prepare(SqliteStmt &stmt, const char *sql, const QVariantList &vars = QVariantList());
    bool prepare(SqliteStmt &stmt, const QString &sql, const QVariantList &vars = QVariantList());
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
    quint32 m_openFlags = 0;
    sqlite3 *m_db = nullptr;
    QString m_filePath;
};

#endif // SQLITEDB_H
