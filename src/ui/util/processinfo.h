#ifndef PROCESSINFO_H
#define PROCESSINFO_H

#include <QObject>

using ProcessHandle = void *;

#define PROC_INVALID_PID    quint32(-1)
#define PROC_INVALID_HANDLE nullptr

class ProcessInfo final
{
public:
    enum OpenFlag : quint8 {
        OpenInfo = 0x00,
        OpenTerminate = 0x01,
    };

    explicit ProcessInfo(quint32 pid = PROC_INVALID_PID, quint8 openFlags = OpenInfo);
    ~ProcessInfo();

    quint32 pid() const { return m_pid; }

    bool isValid() const { return m_pid != PROC_INVALID_PID && m_handle != PROC_INVALID_HANDLE; }

    QString path(bool isKernelPath = false) const;

    bool terminateProcess(int exitCode);

    static quint32 currentPid();

private:
    void openProcess();
    void closeProcess();

private:
    quint8 m_openFlags = 0;
    quint32 m_pid = 0;
    ProcessHandle m_handle = PROC_INVALID_HANDLE;
};

#endif // PROCESSINFO_H
