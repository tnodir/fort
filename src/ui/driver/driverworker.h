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
    void readLogResult(LogBuffer *logBuffer, bool success,
                       const QString &errorMessage);

public slots:
    void readLogAsync(LogBuffer *logBuffer);
    void cancelAsyncIo();
    void abort();

private:
    bool waitLogBuffer();
    void emitReadLogResult(bool success,
                           const QString &errorMessage = QString());

    void readLog();

private:
    volatile bool m_isLogReading;
    volatile bool m_cancelled;
    volatile bool m_aborted;

    Device *m_device;

    LogBuffer *m_logBuffer;

    QMutex m_mutex;
    QWaitCondition m_waitCondition;
};

#endif // DRIVERWORKER_H
