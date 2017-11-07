#ifndef DRIVERWORKER_H
#define DRIVERWORKER_H

#include <QMutex>
#include <QObject>
#include <QRunnable>
#include <QWaitCondition>

class Device;
class LogBuffer;

class DriverWorker : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit DriverWorker(Device *device, QObject *parent = nullptr);

    void run() override;

signals:
    void readLogResult(bool success, const QString &errorMessage);

public slots:
    void readLogAsync(LogBuffer *logBuffer);
    void cancelIo();
    void abort();

private:
    bool waitLogBuffer();

    void readLog();

private:
    volatile bool m_isWorking;
    volatile bool m_cancelled;
    volatile bool m_aborted;

    Device *m_device;

    LogBuffer *m_logBuffer;

    QMutex m_mutex;
    QWaitCondition m_waitCondition;
    QWaitCondition m_cancelledCondition;
};

#endif // DRIVERWORKER_H
