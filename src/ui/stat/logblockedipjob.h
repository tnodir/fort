#ifndef LOGBLOCKEDIPJOB_H
#define LOGBLOCKEDIPJOB_H

#include <QVector>

#include <log/logentryblockedip.h>

#include "statblockbasejob.h"

class StatBlockManager;

class LogBlockedIpJob : public StatBlockBaseJob
{
public:
    explicit LogBlockedIpJob(const LogEntryBlockedIp &entry);

    const QVector<LogEntryBlockedIp> &entries() const { return m_entries; }

    StatBlockJobType jobType() const override { return JobTypeBlockedIp; }

protected:
    bool processMerge(const StatBlockBaseJob &statJob) override;
    void processJob() override;
    void emitFinished() override;

private:
    bool processEntry(const LogEntryBlockedIp &entry);

    qint64 getAppId(const QString &appPath);
    qint64 createAppId(const QString &appPath, qint64 unixTime);
    qint64 getOrCreateAppId(const QString &appPath, qint64 unixTime = 0);

    qint64 insertConn(const LogEntryBlockedIp &entry, qint64 appId);

private:
    qint64 m_connId = 0;

    QVector<LogEntryBlockedIp> m_entries;
};

#endif // LOGBLOCKEDIPJOB_H
