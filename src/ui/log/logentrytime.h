#ifndef LOGENTRYTIME_H
#define LOGENTRYTIME_H

#include "logentry.h"

class LogEntryTime : public LogEntry
{
public:
    explicit LogEntryTime(qint64 unixTime = 0);

    FortLogType type() const override { return FORT_LOG_TYPE_TIME; }

    qint64 unixTime() const { return m_unixTime; }
    void setUnixTime(qint64 unixTime);

private:
    qint64 m_unixTime = 0;
};

#endif // LOGENTRYTIME_H
