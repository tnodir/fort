#ifndef LOGENTRYPROCNEW_H
#define LOGENTRYPROCNEW_H

#include "logentry.h"

class LogEntryProcNew : public LogEntry
{
public:
    explicit LogEntryProcNew(quint32 pid = 0, const QString &kernelPath = QString());

    FortLogType type() const override { return FORT_LOG_TYPE_PROC_NEW; }

    quint32 appId() const { return m_appId; }
    void setAppId(quint32 v) { m_appId = v; }

    quint32 pid() const { return m_pid; }
    void setPid(quint32 pid) { m_pid = pid; }

    QString kernelPath() const { return m_kernelPath; }
    void setKernelPath(const QString &kernelPath) { m_kernelPath = kernelPath; }

    QString path() const;

private:
    quint32 m_appId = 0;
    quint32 m_pid = 0;
    QString m_kernelPath;
};

#endif // LOGENTRYPROCNEW_H
