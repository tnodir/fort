#ifndef STATBLOCKMANAGER_H
#define STATBLOCKMANAGER_H

#include <QObject>

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

    SqliteDb *sqliteDb() const { return m_sqliteDb.data(); }
    SqliteDb *roSqliteDb() const { return m_roSqliteDb.data(); }

    void setUp() override;
    void tearDown() override;

    void logBlockedIp(const LogEntryBlockedIp &entry);

    virtual void deleteConn(qint64 connIdTo = 0);

    static void getConnIdRange(SqliteDb *db, qint64 &rowIdMin, qint64 &rowIdMax);

signals:
    void connChanged();

    void logBlockedIpFinished(int count, qint64 newConnId);
    void deleteConnBlockFinished(qint64 connIdTo);

private:
    void onLogBlockedIpFinished(int count, qint64 newConnId);
    void onDeleteConnBlockFinished(qint64 connIdTo);

    void emitConnChanged();

protected:
    WorkerObject *createWorker() override;
    bool canMergeJobs() const override { return true; }

    virtual void setupWorker();
    virtual void setupConfManager();

private:
    bool setupDb();

    void setupByConf(const IniOptions &ini);

private:
    int m_connInc = 999999999; // to trigger on first check

    int m_keepCount = 0;

    SqliteDbPtr m_sqliteDb;
    SqliteDbPtr m_roSqliteDb;

    TriggerTimer m_connChangedTimer;
};

#endif // STATBLOCKMANAGER_H
