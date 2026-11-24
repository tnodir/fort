#ifndef LOGENTRYPROCNEW_H
#define LOGENTRYPROCNEW_H

#include "logentryprocstart.h"

class LogEntryProcNew : public LogEntryProcStart
{
public:
    explicit LogEntryProcNew(quint32 pid = 0, const QString &kernelPath = QString());

    FortLogType type() const override { return FORT_LOG_TYPE_PROC_NEW; }

    quint32 appId() const { return m_appId; }
    void setAppId(quint32 v) { m_appId = v; }

    QString inheritKernelPath() const { return m_inheritKernelPath; }
    void setInheritKernelPath(const QString &v) { m_inheritKernelPath = v; }

    QString inheritPath() const;

private:
    quint32 m_appId = 0;
    QString m_inheritKernelPath;
};

#endif // LOGENTRYPROCNEW_H
