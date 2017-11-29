#ifndef LOGENTRY_H
#define LOGENTRY_H

#include <QObject>

class LogEntry
{
public:
    enum LogType {
        TypeNone    = -1,
        // synchronize with FORT_LOG_FLAG_*
        AppBlocked  = 0x01000000,
        ProcNew     = 0x02000000,
        StatTraf    = 0x04000000
    };

    explicit LogEntry();
    virtual ~LogEntry();

    virtual LogEntry::LogType type() const = 0;
};

#endif // LOGENTRY_H
