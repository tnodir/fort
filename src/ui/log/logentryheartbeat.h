#ifndef LOGENTRYHEARBEAT_H
#define LOGENTRYHEARBEAT_H

#include "logentry.h"

class LogEntryHeartbeat : public LogEntry
{
public:
    explicit LogEntryHeartbeat(quint16 tick = 0);

    LogEntry::LogType type() const override { return Heartbeat; }

    quint16 tick() const { return m_tick; }
    void setTick(quint16 tick);

private:
    quint16 m_tick = 0;
};

#endif // LOGENTRYHEARBEAT_H
