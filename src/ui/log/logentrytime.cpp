#include "logentrytime.h"

LogEntryTime::LogEntryTime(qint64 unixTime) : m_unixTime(unixTime) { }

void LogEntryTime::setTimeChanged(bool timeChanged)
{
    m_timeChanged = timeChanged;
}

void LogEntryTime::setUnixTime(qint64 unixTime)
{
    m_unixTime = unixTime;
}
