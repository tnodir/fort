#ifndef LOGCONNJOB_H
#define LOGCONNJOB_H

#include <QVector>

#include <log/logentryconn.h>

#include "statconnbasejob.h"

class StatConnManager;

class LogConnJob : public StatConnBaseJob
{
public:
    explicit LogConnJob(const LogEntryConn &entry);

    const QVector<LogEntryConn> &entries() const { return m_entries; }

    StatConnJobType jobType() const override { return JobTypeLogConn; }

protected:
    bool processMerge(const StatConnBaseJob &statJob) override;
    void processJob() override;
    void emitFinished() override;

private:
    bool processEntry(const LogEntryConn &entry);

    qint64 getAppId(const QString &appPath);
    qint64 createAppId(const QString &appPath, quint32 confAppId, qint64 unixTime);
    qint64 getOrCreateAppId(const QString &appPath, quint32 confAppId, qint64 unixTime = 0);

    qint64 insertConn(const LogEntryConn &entry, qint64 appId);

private:
    qint64 m_connId = 0;

    QVector<LogEntryConn> m_entries;
};

#endif // LOGCONNJOB_H
