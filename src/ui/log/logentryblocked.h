#ifndef LOGENTRYBLOCKED_H
#define LOGENTRYBLOCKED_H

#include "logentry.h"

class LogEntryBlocked : public LogEntry
{
public:
    explicit LogEntryBlocked(quint32 pid = 0, const QString &kernelPath = QString());

    FortLogType type() const override;

    bool blocked() const { return m_blocked; }
    void setBlocked(bool blocked) { m_blocked = blocked; }

    bool alerted() const { return m_alerted; }
    void setAlerted(bool alerted) { m_alerted = alerted; }

    quint32 pid() const { return m_pid; }
    void setPid(quint32 pid) { m_pid = pid; }

    QString kernelPath() const { return m_kernelPath; }
    void setKernelPath(const QString &kernelPath) { m_kernelPath = kernelPath; }

    QString path() const;

private:
    bool m_blocked : 1 = true;
    bool m_alerted : 1 = true;
    quint32 m_pid = 0;
    QString m_kernelPath;
};

#endif // LOGENTRYBLOCKED_H
