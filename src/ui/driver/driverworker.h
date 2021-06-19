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
    void readLogResult(LogBuffer *logBuffer, bool success, quint32 errorCode);

public slots:
    bool readLogAsync(LogBuffer *logBuffer);
    bool cancelAsyncIo();
    void continueAsyncIo();
    void close();

private:
    bool waitLogBuffer();
    void emitReadLogResult(bool success, quint32 errorCode = 0);

    void readLog();

private:
    volatile bool m_isLogReading = false;
    volatile bool m_cancelled = false;
    volatile bool m_aborted = false;

    Device *m_device = nullptr;

    LogBuffer *m_logBuffer = nullptr;

    QMutex m_mutex;
    QWaitCondition m_bufferWaitCondition;
    QWaitCondition m_cancelledWaitCondition;
};

#endif // DRIVERWORKER_H
