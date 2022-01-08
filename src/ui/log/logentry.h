#ifndef LOGENTRY_H
#define LOGENTRY_H

#include <QObject>

#include <common/fortdef.h>
#include <util/classhelpers.h>

class LogEntry
{
public:
    explicit LogEntry() = default;
    virtual ~LogEntry() = default;
    CLASS_DEFAULT_COPY_MOVE(LogEntry)

    virtual FortLogType type() const = 0;

protected:
    static QString getAppPath(const QString &kernelPath, quint32 pid);
};

#endif // LOGENTRY_H
