#ifndef LOGBLOCKEDIPJOB_H
#define LOGBLOCKEDIPJOB_H

#include <log/logentryblockedip.h>

#include "statblockbasejob.h"

class StatBlockManager;

class LogBlockedIpJob : public StatBlockBaseJob
{
public:
    explicit LogBlockedIpJob(qint64 unixTime);

    qint64 unixTime() const { return m_unixTime; }
    LogEntryBlockedIp &entry() { return m_entry; }

protected:
    void processJob() override;
    void emitFinished() override;

private:
    qint64 getAppId(const QString &appPath);
    qint64 createAppId(const QString &appPath, qint64 unixTime);
    qint64 getOrCreateAppId(const QString &appPath, qint64 unixTime = 0);

    qint64 insertConn(const LogEntryBlockedIp &entry, qint64 unixTime, qint64 appId);

private:
    const qint64 m_unixTime;

    qint64 m_connId = 0;

    LogEntryBlockedIp m_entry;
};

#endif // LOGBLOCKEDIPJOB_H
