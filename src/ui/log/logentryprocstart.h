#ifndef LOGENTRYPROCSTART_H
#define LOGENTRYPROCSTART_H

#include "logentry.h"

class LogEntryProcStart : public LogEntry
{
public:
    explicit LogEntryProcStart(quint32 pid = 0, const QString &kernelPath = QString());

    FortLogType type() const override { return FORT_LOG_TYPE_PROC_START; }

    quint32 pid() const { return m_pid; }
    void setPid(quint32 pid) { m_pid = pid; }

    quint32 parentPid() const { return m_parentPid; }
    void setParentPid(quint32 v) { m_parentPid = v; }

    qint64 createTime() const { return m_createTime; }
    void setCreateTime(qint64 v) { m_createTime = v; }

    QString kernelPath() const { return m_kernelPath; }
    void setKernelPath(const QString &kernelPath) { m_kernelPath = kernelPath; }

    QString path() const;

private:
    quint32 m_pid = 0;
    quint32 m_parentPid = 0;
    qint64 m_createTime = 0;
    QString m_kernelPath;
};

#endif // LOGENTRYPROCSTART_H
