#ifndef LOGENTRY_H
#define LOGENTRY_H

#include <QObject>

#include <util/classhelpers.h>

class LogEntry
{
public:
    enum LogType {
        TypeNone = -1,
        // synchronize with FORT_LOG_FLAG_*
        AppBlocked = 0x00100000,
        AppBlockedIp = 0x00200000,
        ProcNew = 0x01000000,
        StatTraf = 0x02000000,
        Time = 0x04000000
    };

    explicit LogEntry() = default;
    virtual ~LogEntry() = default;
    CLASS_DEFAULT_COPY_MOVE(LogEntry)

    virtual LogEntry::LogType type() const = 0;

protected:
    static QString getAppPath(const QString &kernelPath, quint32 pid);
};

#endif // LOGENTRY_H
