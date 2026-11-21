#include "logentrystattraf.h"

LogEntryStatTraf::LogEntryStatTraf(quint16 procCount, const quint32 *procTrafBytes) :
    m_procCount(procCount), m_procTrafBytes(procTrafBytes)
{
}

void LogEntryStatTraf::setProcCount(quint16 procCount)
{
    m_procCount = procCount;
}

void LogEntryStatTraf::setProcTrafBytes(const quint32 *procTrafBytes)
{
    m_procTrafBytes = procTrafBytes;
}

LogEntryStatTraf::ProcTraf LogEntryStatTraf::procTraf(int index) const
{
    Q_ASSERT(index >= 0 && index < m_procCount);

    const quint32 *procTrafBytes = m_procTrafBytes + index * 3;
    const quint32 pidFlag = procTrafBytes[0];

    return {
        .inactive = (pidFlag & 1) != 0,
        .pid = pidFlag & ~quint32(1),
        .inBytes = procTrafBytes[1],
        .outBytes = procTrafBytes[2],
    };
}
