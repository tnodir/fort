#include "logentrytime.h"

LogEntryTime::LogEntryTime(qint64 unixTime) : m_unixTime(unixTime) { }

void LogEntryTime::setSystemTimeChanged(bool systemTimeChanged)
{
    m_systemTimeChanged = systemTimeChanged;
}

void LogEntryTime::setUnixTime(qint64 unixTime)
{
    m_unixTime = unixTime;
}
