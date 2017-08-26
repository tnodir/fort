#include "processinfo.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>  /* GetProcessImageFileName */

#define PROC_PATH_MAX    65536

ProcessInfo::ProcessInfo(quint32 pid, QObject *parent) :
    QObject(parent),
    m_pid(pid),
    m_handle(PROC_INVALID_HANDLE)
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
        const DWORD access = (PROCESS_QUERY_INFORMATION | PROCESS_VM_READ);

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

QString ProcessInfo::dosPath() const
{
    if (isValid()) {
        QByteArray buf(PROC_PATH_MAX * sizeof(wchar_t), Qt::Uninitialized);
        wchar_t *p = (wchar_t *) buf.data();

        const DWORD len = GetProcessImageFileNameW(
                    m_handle, (LPWSTR) p, PROC_PATH_MAX);
        if (len) {
            return QString::fromWCharArray(p, len);
        }
    }

    return QString();
}
