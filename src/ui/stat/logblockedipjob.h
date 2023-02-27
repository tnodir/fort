#ifndef LOGBLOCKEDIPJOB_H
#define LOGBLOCKEDIPJOB_H

#include <log/logentryblockedip.h>
#include <util/worker/workerjob.h>

class StatBlockManager;

class LogBlockedIpJob : public WorkerJob
{
public:
    explicit LogBlockedIpJob(qint64 unixTime);

    LogEntryBlockedIp &entry() { return m_entry; }

    void doJob(WorkerObject *worker) override;
    void reportResult(WorkerObject *worker) override;

private:
    void emitFinished(StatBlockManager *manager);

private:
    const qint64 m_unixTime;

    LogEntryBlockedIp m_entry;
};

#endif // LOGBLOCKEDIPJOB_H
