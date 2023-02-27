#ifndef STATBLOCKMANAGER_H
#define STATBLOCKMANAGER_H

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QVector>

#include <sqlite/sqlitetypes.h>

#include <util/classhelpers.h>
#include <util/ioc/iocservice.h>
#include <util/triggertimer.h>
#include <util/worker/workermanager.h>

class LogEntryBlockedIp;

class StatBlockManager : public WorkerManager, public IocService
{
    Q_OBJECT

public:
    explicit StatBlockManager(
            const QString &filePath, QObject *parent = nullptr, quint32 openFlags = 0);
    CLASS_DELETE_COPY_MOVE(StatBlockManager)

    qint64 connBlockIdMin() const { return m_connBlockIdMin; }
    qint64 connBlockIdMax() const { return m_connBlockIdMax; }

    SqliteDb *sqliteDb() const { return m_sqliteDb; }

    void setUp() override;

    void updateConnBlockId();

    bool logBlockedIp(const LogEntryBlockedIp &entry, qint64 unixTime);

    virtual bool deleteConn(qint64 rowIdTo);
    virtual bool deleteConnAll();

signals:
    void connChanged();

protected:
    bool isConnIdRangeUpdated() const { return m_isConnIdRangeUpdated; }
    void setIsConnIdRangeUpdated(bool v) { m_isConnIdRangeUpdated = v; }

private:
    void emitConnChanged();

private:
    bool deleteOldConnBlock();

    qint64 getAppId(const QString &appPath);
    qint64 createAppId(const QString &appPath, qint64 unixTime);
    qint64 getOrCreateAppId(const QString &appPath, qint64 unixTime = 0);
    bool deleteAppId(qint64 appId);

    qint64 insertConn(const LogEntryBlockedIp &entry, qint64 unixTime, qint64 appId);
    qint64 insertConnBlock(qint64 connId, quint8 blockReason);

    bool createConnBlock(const LogEntryBlockedIp &entry, qint64 unixTime, qint64 appId);
    void deleteConnBlock(qint64 rowIdTo);

    void deleteAppStmtList(const SqliteStmtList &stmtList, SqliteStmt *stmtAppList);

    SqliteStmt *getStmt(const char *sql);
    SqliteStmt *getTrafficStmt(const char *sql, qint32 trafTime);
    SqliteStmt *getIdStmt(const char *sql, qint64 id);

protected:
    WorkerObject *createWorker() override;

private:
    bool m_isConnIdRangeUpdated : 1;

    int m_connBlockInc = 999999999; // to trigger on first check

    qint64 m_connBlockIdMin = 0;
    qint64 m_connBlockIdMax = 0;

    SqliteDb *m_sqliteDb = nullptr;

    QHash<quint32, QString> m_appPidPathMap; // pid -> appPath
    QHash<QString, qint64> m_appPathIdCache; // appPath -> appId

    TriggerTimer m_connChangedTimer;
};

#endif // STATBLOCKMANAGER_H
