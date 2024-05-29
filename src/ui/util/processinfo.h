#ifndef PROCESSINFO_H
#define PROCESSINFO_H

#include <QObject>

using ProcessHandle = void *;

#define PROC_INVALID_PID    quint32(-1)
#define PROC_INVALID_HANDLE nullptr

class ProcessInfo final
{
public:
    explicit ProcessInfo(quint32 pid = PROC_INVALID_PID);
    ~ProcessInfo();

    quint32 pid() const { return m_pid; }

    bool isValid() const { return m_pid != PROC_INVALID_PID && m_handle != PROC_INVALID_HANDLE; }

    QString path(bool isKernelPath = false) const;

    static quint32 currentPid();

private:
    void openProcess();
    void closeProcess();

private:
    quint32 m_pid = 0;
    ProcessHandle m_handle = PROC_INVALID_HANDLE;
};

#endif // PROCESSINFO_H
