#ifndef LOGENTRYSTATTRAF_H
#define LOGENTRYSTATTRAF_H

#include "logentry.h"

class LogEntryStatTraf : public LogEntry
{
public:
    explicit LogEntryStatTraf(quint16 procCount = 0, const quint32 *procTrafBytes = nullptr);

    FortLogType type() const override { return FORT_LOG_TYPE_STAT_TRAF; }

    quint16 procCount() const { return m_procCount; }
    void setProcCount(quint16 procCount);

    const quint32 *procTrafBytes() const { return m_procTrafBytes; }
    void setProcTrafBytes(const quint32 *procTrafBytes);

private:
    quint16 m_procCount = 0;
    const quint32 *m_procTrafBytes = nullptr;
};

#endif // LOGENTRYSTATTRAF_H
