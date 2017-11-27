#ifndef LOGENTRYSTATTRAF_H
#define LOGENTRYSTATTRAF_H

#include "logentry.h"

class LogEntryStatTraf : public LogEntry
{
public:
    explicit LogEntryStatTraf(quint16 procCount = 0,
                              const quint32 *trafBytes = nullptr);

    virtual LogEntry::LogType type() const { return StatTraf; }

    quint16 procCount() const { return m_procCount; }
    void setProcCount(quint16 procCount);

    const quint32 *trafBytes() const { return m_trafBytes; }
    void setTrafBytes(const quint32 *trafBytes);

private:
    quint16 m_procCount;
    const quint32 *m_trafBytes;
};

#endif // LOGENTRYSTATTRAF_H
