#include "logentrystattraf.h"

LogEntryStatTraf::LogEntryStatTraf(quint16 procCount,
                                   const quint32 *trafBytes) :
    LogEntry(),
    m_procCount(procCount),
    m_trafBytes(trafBytes)
{
}

void LogEntryStatTraf::setProcCount(quint16 procCount)
{
    m_procCount = procCount;
}

void LogEntryStatTraf::setTrafBytes(const quint32 *trafBytes)
{
    m_trafBytes = trafBytes;
}
