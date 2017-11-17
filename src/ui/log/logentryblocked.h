#ifndef LOGENTRYBLOCKED_H
#define LOGENTRYBLOCKED_H

#include "logentry.h"

class LogEntryBlocked : public LogEntry
{
public:
    explicit LogEntryBlocked(quint32 ip = 0, quint32 pid = 0,
                             const QString &kernelPath = QString());

    virtual LogEntry::LogType type() const { return AppBlocked; }

    quint32 ip() const { return m_ip; }
    void setIp(quint32 ip);

    quint32 pid() const { return m_pid; }
    void setPid(quint32 pid);

    QString kernelPath() const { return m_kernelPath; }
    void setKernelPath(const QString &kernelPath);

private:
    quint32 m_ip;
    quint32 m_pid;
    QString m_kernelPath;
};

#endif // LOGENTRYBLOCKED_H
