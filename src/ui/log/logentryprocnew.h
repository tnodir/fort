#ifndef LOGENTRYPROCNEW_H
#define LOGENTRYPROCNEW_H

#include "logentry.h"

class LogEntryProcNew : public LogEntry
{
public:
    explicit LogEntryProcNew(quint32 pid = 0, const QString &kernelPath = QString());

    FortLogType type() const override { return FORT_LOG_TYPE_PROC_NEW; }

    quint32 pid() const { return m_pid; }
    void setPid(quint32 pid);

    QString kernelPath() const { return m_kernelPath; }
    void setKernelPath(const QString &kernelPath);

    QString path() const;

private:
    quint32 m_pid = 0;
    QString m_kernelPath;
};

#endif // LOGENTRYPROCNEW_H
