#ifndef LOGENTRYPROCEXIT_H
#define LOGENTRYPROCEXIT_H

#include "logentry.h"

class LogEntryProcExit : public LogEntry
{
public:
    explicit LogEntryProcExit(quint32 pid = 0);

    FortLogType type() const override { return FORT_LOG_TYPE_PROC_EXIT; }

    quint32 pid() const { return m_pid; }
    void setPid(quint32 pid) { m_pid = pid; }

private:
    quint32 m_pid = 0;
};

#endif // LOGENTRYPROCEXIT_H
