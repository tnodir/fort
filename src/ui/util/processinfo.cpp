#include "processinfo.h"

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>
#include <winternl.h>

#define PROC_PATH_MAX 65536

namespace {

// Windows 8.1+: a UNICODE_STRING followed by its buffer
inline constexpr auto ProcessCommandLineInformation = PROCESSINFOCLASS(60);

QByteArray queryProcessInfo(ProcessHandle handle, PROCESSINFOCLASS infoClass)
{
    // The size is returned by the first call
    ULONG len = 0;
    NtQueryInformationProcess(handle, infoClass, nullptr, 0, &len);
    if (len == 0)
        return {};

    QByteArray buf(len, Qt::Uninitialized);

    if (!NT_SUCCESS(NtQueryInformationProcess(handle, infoClass, buf.data(), len, &len)))
        return {};

    return buf;
}

}

ProcessInfo::ProcessInfo(quint32 pid, quint8 openFlags) : m_openFlags(openFlags), m_pid(pid)
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
        const DWORD access = PROCESS_QUERY_LIMITED_INFORMATION
                | ((m_openFlags & OpenTerminate) != 0 ? PROCESS_TERMINATE : 0);

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

qint64 ProcessInfo::createTime() const
{
    FILETIME creationTime, exitTime, kernelTime, userTime;

    if (!isValid() || !GetProcessTimes(m_handle, &creationTime, &exitTime, &kernelTime, &userTime))
        return 0;

    return (qint64(creationTime.dwHighDateTime) << 32) | creationTime.dwLowDateTime;
}

QString ProcessInfo::commandLine() const
{
    if (!isValid())
        return {};

    const QByteArray buf = queryProcessInfo(m_handle, ProcessCommandLineInformation);
    if (buf.isEmpty())
        return {};

    const auto cmdLine = reinterpret_cast<const UNICODE_STRING *>(buf.constData());

    return QString::fromWCharArray(cmdLine->Buffer, cmdLine->Length / sizeof(wchar_t));
}

bool ProcessInfo::terminateProcess(int exitCode)
{
    return TerminateProcess(m_handle, exitCode) != 0;
}
