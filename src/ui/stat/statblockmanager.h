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

class IniOptions;
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
    SqliteDb *roSqliteDb() const { return m_roSqliteDb; }

    void setUp() override;

    void updateConnBlockId();

    bool logBlockedIp(const LogEntryBlockedIp &entry, qint64 unixTime);

    virtual void deleteConn(qint64 connIdTo = 0);

signals:
    void connChanged();

    void logBlockedIpFinished(int count, qint64 newConnId);
    void deleteConnBlockFinished(qint64 connIdTo);

protected:
    bool isConnIdRangeUpdated() const { return m_isConnIdRangeUpdated; }
    void setIsConnIdRangeUpdated(bool v) { m_isConnIdRangeUpdated = v; }

private:
    void onLogBlockedIpFinished(int count, qint64 newConnId);
    void onDeleteConnBlockFinished(qint64 connIdTo);

    void emitConnChanged();

protected:
    WorkerObject *createWorker() override;

    virtual void setupWorker();
    virtual void setupConfManager();

private:
    void setupByConf(const IniOptions &ini);

private:
    bool m_isConnIdRangeUpdated : 1;

    int m_connBlockInc = 999999999; // to trigger on first check

    int m_blockedIpKeepCount = 0;

    qint64 m_connBlockIdMin = 0;
    qint64 m_connBlockIdMax = 0;

    SqliteDb *m_sqliteDb = nullptr;
    SqliteDb *m_roSqliteDb = nullptr;

    TriggerTimer m_connChangedTimer;
};

#endif // STATBLOCKMANAGER_H
