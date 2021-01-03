#ifndef LOGENTRYTIME_H
#define LOGENTRYTIME_H

#include "logentry.h"

class LogEntryTime : public LogEntry
{
public:
    explicit LogEntryTime(qint64 unixTime = 0);

    LogEntry::LogType type() const override { return Time; }

    qint64 unixTime() const { return m_unixTime; }
    void setUnixTime(qint64 unixTime);

private:
    qint64 m_unixTime = 0;
};

#endif // LOGENTRYTIME_H
