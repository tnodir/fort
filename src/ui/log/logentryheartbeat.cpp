#include "logentryheartbeat.h"

LogEntryHeartbeat::LogEntryHeartbeat(quint16 tick) :
    m_tick(tick)
{
}

void LogEntryHeartbeat::setTick(quint16 tick)
{
    m_tick = tick;
}
