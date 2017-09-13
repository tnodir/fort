#ifndef PROCESSINFO_H
#define PROCESSINFO_H

#include <QObject>

typedef void *phandle_t;

#define PROC_INVALID_PID        -1
#define PROC_INVALID_HANDLE     nullptr

class ProcessInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint32 pid READ pid CONSTANT)

public:
    explicit ProcessInfo(quint32 pid = PROC_INVALID_PID,
                         QObject *parent = nullptr);
    virtual ~ProcessInfo();

    quint32 pid() const { return m_pid; }

    bool isValid() const {
        return m_pid != PROC_INVALID_PID
                && m_handle != PROC_INVALID_HANDLE;
    }

    static quint32 currentPid();

signals:

public slots:
    QString kernelPath() const;

private:
    void openProcess();
    void closeProcess();

private:
    quint32 m_pid;
    phandle_t m_handle;
};

#endif // PROCESSINFO_H
