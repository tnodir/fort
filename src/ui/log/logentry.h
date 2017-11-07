#ifndef LOGENTRY_H
#define LOGENTRY_H

#include <QObject>

class LogEntry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(LogType type READ type CONSTANT)

public:
    enum LogType {
        TypeNone    = -1,
        // synchronize with FORT_LOG_FLAG_*
        AppBlocked  = 0x01000000,
        UsageStat   = 0x02000000
    };
    Q_ENUM(LogType)

    explicit LogEntry(QObject *parent = nullptr);

    virtual LogEntry::LogType type() const = 0;
};

#endif // LOGENTRY_H
