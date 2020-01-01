#ifndef LOGENTRYBLOCKED_H
#define LOGENTRYBLOCKED_H

#include "logentry.h"

class LogEntryBlocked : public LogEntry
{
public:
    explicit LogEntryBlocked(quint32 ip = 0, quint16 port = 0,
                             quint8 proto = 0, quint32 pid = 0,
                             const QString &kernelPath = QString());

    LogEntry::LogType type() const override { return AppBlocked; }

    quint8 proto() const { return m_proto; }
    void setProto(quint8 proto);

    quint16 port() const { return m_port; }
    void setPort(quint16 port);

    quint32 ip() const { return m_ip; }
    void setIp(quint32 ip);

    quint32 pid() const { return m_pid; }
    void setPid(quint32 pid);

    QString kernelPath() const { return m_kernelPath; }
    void setKernelPath(const QString &kernelPath);

    QString path() const;

private:
    quint8 m_proto = 0;
    quint16 m_port = 0;
    quint32 m_ip = 0;
    quint32 m_pid = 0;
    QString m_kernelPath;
};

#endif // LOGENTRYBLOCKED_H
