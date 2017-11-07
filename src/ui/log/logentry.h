#ifndef LOGENTRY_H
#define LOGENTRY_H

#include <QObject>

class LogEntry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(LogType type READ type CONSTANT)

public:
    enum LogType {
        TypeNone = -1,
        AppBlocked = 0,
        UsageStat
    };
    Q_ENUM(LogType)

    explicit LogEntry(QObject *parent = nullptr);

    virtual LogEntry::LogType type() const = 0;
};

#endif // LOGENTRY_H
