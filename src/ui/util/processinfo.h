#ifndef PROCESSINFO_H
#define PROCESSINFO_H

#include <QObject>

#include "classhelpers.h"

using phandle_t = void *;

#define PROC_INVALID_PID    quint32(-1)
#define PROC_INVALID_HANDLE nullptr

class ProcessInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint32 pid READ pid CONSTANT)

public:
    explicit ProcessInfo(quint32 pid = PROC_INVALID_PID, QObject *parent = nullptr);
    ~ProcessInfo() override;
    CLASS_DELETE_COPY_MOVE(ProcessInfo)

    quint32 pid() const { return m_pid; }

    bool isValid() const { return m_pid != PROC_INVALID_PID && m_handle != PROC_INVALID_HANDLE; }

    QString path(bool isKernelPath = false) const;

    static quint32 currentPid();

private:
    void openProcess();
    void closeProcess();

private:
    quint32 m_pid = 0;
    phandle_t m_handle = PROC_INVALID_HANDLE;
};

#endif // PROCESSINFO_H
