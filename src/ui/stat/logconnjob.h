#ifndef LOGCONNJOB_H
#define LOGCONNJOB_H

#include <QVector>

#include <log/logentryconn.h>

#include "statconnbasejob.h"

class SqliteStmt;
class StatConnManager;

class LogConnJob : public StatConnBaseJob
{
public:
    explicit LogConnJob(const LogEntryConn &entry);

    const QVector<LogEntryConn> &entries() const { return m_entries; }

    StatConnJobType jobType() const override { return JobTypeLogConn; }

protected:
    /* The parameters of sqlInsertConn() by their numbers */
    enum ConnParam : qint8 {
        ConnAppId = 1,
        ConnTime,
        ConnProcessId,
        ConnReason,
        ConnBlocked,
        ConnDropped,
        ConnAlerted,
        ConnTerminated,
        ConnInherited,
        ConnInbound,
        ConnIpProto,
        ConnLocalPort,
        ConnRemotePort,
        ConnLocalIp,
        ConnRemoteIp,
        ConnLocalIp6,
        ConnRemoteIp6,
        ConnZoneId,
        ConnRuleId,
        ConnLoopback,
        ConnInheritAppId,
        ConnParamEnd, // a variant's parameters follow
    };

    bool processMerge(const StatConnBaseJob &statJob) override;
    void processJob() override;
    void emitFinished() override;

    virtual const char *sqlInsertConn() const;
    virtual void bindConn(
            SqliteStmt *stmt, const LogEntryConn &entry, qint64 appId, qint64 inheritAppId) const;

private:
    bool processEntry(const LogEntryConn &entry);

    qint64 getAppId(const QString &appPath);
    qint64 createAppId(const QString &appPath, quint32 confAppId, qint64 unixTime);
    qint64 getOrCreateAppId(const QString &appPath, quint32 confAppId, qint64 unixTime = 0);
    qint64 getOrCreateInheritAppId(const LogEntryConn &entry);

    qint64 insertConn(const LogEntryConn &entry, qint64 appId, qint64 inheritAppId);

private:
    qint64 m_connId = 0;

    QVector<LogEntryConn> m_entries;
};

#endif // LOGCONNJOB_H
