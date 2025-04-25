#ifndef STATCONNMANAGER_H
#define STATCONNMANAGER_H

#include <QObject>

#include <sqlite/sqlite_types.h>

#include <manager/dbmanagerbase.h>
#include <util/classhelpers.h>
#include <util/ioc/iocservice.h>
#include <util/triggertimer.h>
#include <util/worker/workermanager.h>

class IniOptions;
class LogEntryConn;

class StatConnManager : public WorkerManager, public DbManagerBase, public IocService
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

    void logConn(const LogEntryConn &entry);

    virtual void deleteConn(qint64 connIdTo = 0);

    static void getConnIdRange(SqliteDb *db, qint64 &rowIdMin, qint64 &rowIdMax);

signals:
    void connChanged();

    void logConnFinished(int count, qint64 newConnId);
    void deleteConnFinished(qint64 connIdTo);

private:
    void onLogConnFinished(int count, qint64 newConnId);
    void onDeleteConnFinished(qint64 connIdTo);

    void emitConnChanged();

protected:
    WorkerObject *createWorker() override;
    bool canMergeJobs() const override { return true; }

    virtual void setupWorker();
    virtual void setupConfManager();

    virtual void checkCearConnOnExit();

private:
    bool setupDb();

    void setupByConf();
    void setupByConfIni(const IniOptions &ini);

private:
    bool m_logAllowedConn = false;
    bool m_logBlockedConn = false;

    int m_keepCount = 0;

    int m_connInc = 999999999; // to trigger on first check

    SqliteDbPtr m_sqliteDb;
    SqliteDbPtr m_roSqliteDb;

    TriggerTimer m_connChangedTimer;
};

#endif // STATCONNMANAGER_H
