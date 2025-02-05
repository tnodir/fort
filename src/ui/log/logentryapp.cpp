#include "logentryapp.h"

LogEntryApp::LogEntryApp(quint32 pid, const QString &kernelPath) :
    m_pid(pid), m_kernelPath(kernelPath)
{
}

FortLogType LogEntryApp::type() const
{
    return blocked() ? FORT_LOG_TYPE_BLOCKED : FORT_LOG_TYPE_ALLOWED;
}

QString LogEntryApp::path() const
{
    return getAppPath(m_kernelPath, m_pid);
}
