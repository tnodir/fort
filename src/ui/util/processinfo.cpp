#include "processinfo.h"

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#define PROC_PATH_MAX 65536

ProcessInfo::ProcessInfo(quint32 pid, QObject *parent) : QObject(parent), m_pid(pid)
{
    openProcess();
}

ProcessInfo::~ProcessInfo()
{
    closeProcess();
}

quint32 ProcessInfo::currentPid()
{
    return GetCurrentProcessId();
}

void ProcessInfo::openProcess()
{
    if (m_pid == PROC_INVALID_PID)
        return;

    if (m_pid == currentPid()) {
        m_handle = GetCurrentProcess();
    } else {
        const DWORD access = PROCESS_QUERY_LIMITED_INFORMATION;

        m_handle = OpenProcess(access, FALSE, m_pid);
    }
}

void ProcessInfo::closeProcess()
{
    if (isValid()) {
        CloseHandle(m_handle);
        m_handle = PROC_INVALID_HANDLE;
    }
}

QString ProcessInfo::path(bool isKernelPath) const
{
    if (isValid()) {
        QByteArray buf(PROC_PATH_MAX * sizeof(wchar_t), Qt::Uninitialized);
        wchar_t *p = reinterpret_cast<wchar_t *>(buf.data());
        const DWORD flags = (isKernelPath ? PROCESS_NAME_NATIVE : 0);
        DWORD len = PROC_PATH_MAX;

        if (QueryFullProcessImageNameW(m_handle, flags, (LPWSTR) p, &len)) {
            return QString::fromWCharArray(p, int(len));
        }
    }

    return QString();
}
