#ifndef STATCONNMANAGER_H
#define STATCONNMANAGER_H

#include <QObject>
#include <QVector>

#include <sqlite/sqlite_types.h>
#include <sqlite/sqliteutilbase.h>

#include <util/classhelpers.h>
#include <util/expiringhash.h>
#include <util/ioc/iocservice.h>
#include <util/net/processipkey.h>
#include <util/triggertimer.h>
#include <util/worker/workermanager.h>

class DeleteConnIdsJob;
class IniOptions;
class LogConnJob;
class LogEntryConn;

class StatConnManager : public WorkerManager, public SqliteUtilBase, public IocService
{
    Q_OBJECT

public:
    explicit StatConnManager(
            const QString &filePath, QObject *parent = nullptr, quint32 openFlags = 0);
    CLASS_DELETE_COPY_MOVE(StatConnManager)

    SqliteDb *sqliteDb() const override { return m_sqliteDb.data(); }
    SqliteDb *roSqliteDb() const { return m_roSqliteDb.data(); }

    void setUp() override;
    void tearDown() override;

    virtual bool logConn(const LogEntryConn &entry, bool urgent = false);

    /* Names the process's connections to the address, logged without a name before the DNS
     * Client's answer */
    void updateConnHost(const ProcessIpKey &key, const QString &name);

    virtual void deleteConn(qint64 connIdTo = 0);
    virtual void deleteConns(const QVector<qint64> &connIds);

    static void getConnIdRange(SqliteDb *db, qint64 &rowIdMin, qint64 &rowIdMax);

signals:
    void connChanged();
    void connCleared();
    void connIdsShifted(); // the later connections' ids, after deleting by ids
    void connHostsChanged(qint64 connIdMin); // the host names of the connections after the id

    void logConnFinished(int count, qint64 newConnId);
    void deleteConnFinished(qint64 connIdTo);

private:
    void onLogConnFinished(int count, qint64 newConnId);
    void onDeleteConnFinished(qint64 connIdTo);

    void waitConnHost(const LogEntryConn &entry);
    void resetHostWaitsConnIds();

protected:
    void emitConnChanged();

    WorkerObject *createWorker() override;
    bool canMergeJobs() const override { return true; }

    virtual void setupWorker();
    virtual void setupConfManager();

    virtual bool canLogConn(const LogEntryConn &entry) const;
    virtual LogConnJob *createLogConnJob(const LogEntryConn &entry) const;

    /* Shifts the later connections' ids down: ConnListModel lists the contiguous ids */
    virtual DeleteConnIdsJob *createDeleteConnIdsJob(const QVector<qint64> &connIds) const;

    virtual const char *migrationsSqlDir() const;

    virtual void checkClearConnOnStartup();
    virtual void checkClearConnOnExit();

private:
    bool setupDb();

    void setupByConf();
    void setupByConfIni();

private:
    bool m_logAllowedConn = false;
    bool m_logBlockedConn = false;

    bool m_clearOnExit = false;

    int m_keepCount = 0;

    int m_connInc = 999999999; // to trigger on first check

    qint64 m_lastConnId = 0; // the newest logged connection's id

    SqliteDbPtr m_sqliteDb;
    SqliteDbPtr m_roSqliteDb;

    // The connections logged without a name: after the id
    ExpiringHash<ProcessIpKey, qint64> m_hostWaits;

    TriggerTimer m_connChangedTimer;
};

#endif // STATCONNMANAGER_H
