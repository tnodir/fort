#include "logentrytime.h"

LogEntryTime::LogEntryTime(qint64 unixTime) : m_unixTime(unixTime) { }

void LogEntryTime::setUnixTime(qint64 unixTime)
{
    m_unixTime = unixTime;
}
