#ifndef LOGENTRYPROC_H
#define LOGENTRYPROC_H

#include "logentry.h"

// The process's entry
class LogEntryProc : public LogEntry
{
public:
    explicit LogEntryProc(quint32 pid = 0) : m_pid(pid) { }

    quint32 pid() const { return m_pid; }
    void setPid(quint32 pid) { m_pid = pid; }

private:
    quint32 m_pid = 0;
};

#endif // LOGENTRYPROC_H
