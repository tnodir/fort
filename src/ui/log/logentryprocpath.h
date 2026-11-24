#ifndef LOGENTRYPROCPATH_H
#define LOGENTRYPROCPATH_H

#include "logentryproc.h"

// The process's entry with its path
class LogEntryProcPath : public LogEntryProc
{
public:
    explicit LogEntryProcPath(quint32 pid = 0, const QString &kernelPath = {});

    QString kernelPath() const { return m_kernelPath; }
    void setKernelPath(const QString &kernelPath) { m_kernelPath = kernelPath; }

    QString path() const;

private:
    QString m_kernelPath;
};

#endif // LOGENTRYPROCPATH_H
