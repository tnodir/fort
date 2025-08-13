#ifndef LOGENTRYPROCKILL_H
#define LOGENTRYPROCKILL_H

#include "logentry.h"

class LogEntryProcKill : public LogEntry
{
public:
    explicit LogEntryProcKill(quint32 pid = 0);

    FortLogType type() const override { return FORT_LOG_TYPE_PROC_KILL; }

    quint32 pid() const { return m_pid; }
    void setPid(quint32 pid) { m_pid = pid; }

private:
    quint32 m_pid = 0;
};

#endif // LOGENTRYPROCKILL_H
