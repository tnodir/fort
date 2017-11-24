#ifndef LOGENTRYPROCDEL_H
#define LOGENTRYPROCDEL_H

#include "logentry.h"

class LogEntryProcDel : public LogEntry
{
public:
    explicit LogEntryProcDel(quint32 pid = 0);

    virtual LogEntry::LogType type() const { return ProcDel; }

    quint32 pid() const { return m_pid; }
    void setPid(quint32 pid);

private:
    quint32 m_pid;
};

#endif // LOGENTRYPROCDEL_H
