#include "logentryapp.h"

LogEntryApp::LogEntryApp(quint32 pid, const QString &kernelPath) :
    m_pid(pid), m_kernelPath(kernelPath)
{
}

QString LogEntryApp::path() const
{
    return getAppPath(m_kernelPath, m_pid);
}
