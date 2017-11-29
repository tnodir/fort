#include "logentrystattraf.h"

LogEntryStatTraf::LogEntryStatTraf(quint16 procCount,
                                   const quint8 *procBits,
                                   const quint32 *trafBytes) :
    LogEntry(),
    m_procCount(procCount),
    m_procBits(procBits),
    m_trafBytes(trafBytes)
{
}

void LogEntryStatTraf::setProcCount(quint16 procCount)
{
    m_procCount = procCount;
}

void LogEntryStatTraf::setProcBits(const quint8 *procBits)
{
    m_procBits = procBits;
}

void LogEntryStatTraf::setTrafBytes(const quint32 *trafBytes)
{
    m_trafBytes = trafBytes;
}
