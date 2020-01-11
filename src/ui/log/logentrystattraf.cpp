#include "logentrystattraf.h"

LogEntryStatTraf::LogEntryStatTraf(quint16 procCount,
                                   const quint32 *procTrafBytes) :
    m_procCount(procCount),
    m_procTrafBytes(procTrafBytes)
{
}

void LogEntryStatTraf::setProcCount(quint16 procCount)
{
    m_procCount = procCount;
}

void LogEntryStatTraf::setUnixTime(qint64 unixTime)
{
    m_unixTime = unixTime;
}

void LogEntryStatTraf::setProcTrafBytes(const quint32 *procTrafBytes)
{
    m_procTrafBytes = procTrafBytes;
}
