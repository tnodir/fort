#ifndef ACTIVITYMANAGER_H
#define ACTIVITYMANAGER_H

#include <QHash>
#include <QObject>

#include <sqlite/sqlite_types.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqliteutilbase.h>

#include <util/classhelpers.h>
#include <util/expiringhash.h>
#include <util/ioc/iocservice.h>
#include <util/net/processipkey.h>

class LogEntryConn;
class LogEntryFlowClosed;
class LogEntryFlowTraf;
class LogEntryProcExit;
class LogEntryProcNew;
class LogEntryProcStart;
class LogEntryStatTraf;

// The live processes and their allowed flows, for the Activity window: the master keeps them in
// the DB (the `proc` and `flow` tables), a client opens it read-only
class ActivityManager : public QObject, public IocService, public SqliteUtilBase
{
    Q_OBJECT

public:
    explicit ActivityManager(const QString &filePath, QObject *parent = nullptr,
            quint32 openFlags = SqliteDb::OpenDefaultReadWrite);
    CLASS_DELETE_COPY_MOVE(ActivityManager)

    SqliteDb *sqliteDb() const override { return m_sqliteDb.data(); }

    void setUp() override;

    void logConn(const LogEntryConn &entry);
    void logProcNew(const LogEntryProcNew &entry, const QString &path);
    void logProcStart(const LogEntryProcStart &entry);
    void logProcExit(const LogEntryProcExit &entry);
    void logFlowTraf(const LogEntryFlowTraf &entry);
    void logFlowClosed(const LogEntryFlowClosed &entry);
    void logStatTraf(const LogEntryStatTraf &entry);
    void logTime();

    // The processed log buffer's changes
    void commitChanges();

    // The host name resolved after the flows' CONN
    void updateConnHost(const ProcessIpKey &key, const QString &name);

    // The flows aren't logged: the service starts with the old file left opened by a client
    void clear();

    // The live services' processes' command lines by their PIDs
    QHash<quint32, QString> serviceCommandLines() const;

signals:
    // The changes are committed: each log buffer and each late host name at most
    void activityChanged();

private:
    void restoreFlowHosts();
    bool setupDb();

    bool beginChanges();
    bool doneChange(SqliteStmt *stmt);
    bool doneChange(const char *sql, const QVariantList &vars);

    bool insertFlow(const LogEntryConn &entry);
    void upsertConnProc(const LogEntryConn &entry);
    void waitConnHost(const LogEntryConn &entry);
    void updateFlowReason(const LogEntryConn &entry);

    void updateFlowsTraf(const LogEntryFlowTraf &entry);

    void deleteById(const char *sql, quint64 id);

    void updateFlowsHost(const ProcessIpKey &key, const QString &name);

    void clearIdleSpeeds();

private:
    bool m_inChanges = false; // the write transaction is opened, till the log buffer's end
    bool m_changed = false; // the opened transaction has changes
    bool m_hasSpeeds = false; // the flows may have the speeds of the previous tick
    bool m_tickHasSpeeds = false; // the current tick's portions have set the speeds

    qint64 m_tick = 0; // the driver's log timer ticks, by the TIMEs

    SqliteDbPtr m_sqliteDb;

    ExpiringHash<ProcessIpKey, bool> m_hostWaits; // the flows logged without the host name
};

#endif // ACTIVITYMANAGER_H
