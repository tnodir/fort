#ifndef SQLITEDB_H
#define SQLITEDB_H

#include <QHash>
#include <QObject>
#include <QString>
#include <QVariant>

#include <util/classhelpers.h>

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

    struct FtsTable
    {
        const QString contentTable;
        const QString contentRowid;
        const QStringList columns;
    };

    struct MigrateOptions
    {
        const QString sqlDir;
        const char *sqlPragmas = nullptr;
        int version = 0;
        int userVersion = 0;
        bool recreate = true;
        bool importOldData = true;
        bool autoCopyTables = true;
        SQLITEDB_MIGRATE_FUNC migrateFunc = nullptr;
        void *migrateContext = nullptr;
        QVector<FtsTable> ftsTables;
    };

    explicit SqliteDb(
            const QString &filePath = QString(), quint32 openFlags = OpenDefaultReadWrite);
    virtual ~SqliteDb();

    struct sqlite3 *db() const { return m_db; }

    quint32 openFlags() const { return m_openFlags; }
    void setOpenFlags(quint32 v) { m_openFlags = v; }

    QString filePath() const { return m_filePath; }
    void setFilePath(const QString &v) { m_filePath = v; }

    bool open();
    void close();

    bool attach(const QString &schemaName, const QString &filePath = {});
    bool detach(const QString &schemaName);

    bool vacuum();
    bool vacuumInto(const QString &filePath);

    bool execute(const char *sql);
    bool executeStr(const QString &sql);

    QVariant executeEx(const char *sql, const QVariantList &vars = {}, int resultCount = 1,
            bool *ok = nullptr);

    bool executeExOk(const char *sql, const QVariantList &vars = {});

    bool prepare(SqliteStmt &stmt, const char *sql, const QVariantList &vars = {});
    bool prepare(SqliteStmt &stmt, const QString &sql, const QVariantList &vars = {});
    bool done(SqliteStmt *stmt);

    qint64 lastInsertRowid() const;
    int changes() const;

    bool beginTransaction();
    bool beginWriteTransaction();
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

    bool setBusyTimeoutMs(int v);

    static QString getFtsTableName(const QString &tableName);

    static QString migrationOldSchemaName();
    static QString migrationNewSchemaName();
    static QString entityName(const QString &schemaName, const QString &objectName);
    QStringList tableNames(const QString &schemaName = {});
    QStringList columnNames(const QString &tableName, const QString &schemaName = {});

    bool migrate(SqliteDb::MigrateOptions &opt);

    SqliteStmt *stmt(const char *sql);

private:
    bool canMigrate(const MigrateOptions &opt) const;
    bool migrateDb(const MigrateOptions &opt, int userVersion, bool isNewDb);
    bool migrateSqlScripts(const MigrateOptions &opt, int userVersion, bool isNewDb);

    bool migrateDbBegin(const MigrateOptions &opt, int &userVersion, bool &isNewDb);
    bool migrateDbEnd(const MigrateOptions &opt);

    bool createFtsTables(const MigrateOptions &opt);
    bool createFtsTable(const FtsTable &ftsTable);

    bool clearWithBackup(const char *sqlPragmas);
    bool importBackup(const MigrateOptions &opt);

    QString backupFilePath() const;

    bool importDb(const MigrateOptions &opt, const QString &sourceFilePath);
    bool copyTables(const QString &srcSchema, const QString &dstSchema);
    bool copyTable(const QString &srcSchema, const QString &dstSchema, const QString &tableName);

    void clearStmts();

private:
    quint32 m_openFlags = 0;
    sqlite3 *m_db = nullptr;
    QString m_filePath;

    QHash<const char *, SqliteStmt *> m_stmts;
};

#endif // SQLITEDB_H
