#include "logentryprocdel.h"

LogEntryProcDel::LogEntryProcDel(quint32 pid) :
    LogEntry(),
    m_pid(pid)
{
}

void LogEntryProcDel::setPid(quint32 pid)
{
    m_pid = pid;
}
