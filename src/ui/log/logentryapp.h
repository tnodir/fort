#ifndef LOGENTRYAPP_H
#define LOGENTRYAPP_H

#include "logentry.h"

class LogEntryApp : public LogEntry
{
public:
    explicit LogEntryApp(quint32 pid = 0, const QString &kernelPath = {});

    FortLogType type() const override { return FORT_LOG_TYPE_APP; }

    bool blocked() const { return m_blocked; }
    void setBlocked(bool v) { m_blocked = v; }

    bool alerted() const { return m_alerted; }
    void setAlerted(bool v) { m_alerted = v; }

    quint32 pid() const { return m_pid; }
    void setPid(quint32 v) { m_pid = v; }

    QString kernelPath() const { return m_kernelPath; }
    void setKernelPath(const QString &v) { m_kernelPath = v; }

    QString path() const;

private:
    bool m_blocked : 1 = true;
    bool m_alerted : 1 = true;
    quint32 m_pid = 0;
    QString m_kernelPath;
};

#endif // LOGENTRYAPP_H
