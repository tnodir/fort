#ifndef DRIVERWORKER_H
#define DRIVERWORKER_H

#include <QMutex>
#include <QObject>
#include <QWaitCondition>

class Device;
class LogBuffer;

class DriverWorker : public QObject
{
    Q_OBJECT

public:
    explicit DriverWorker(Device *device, QObject *parent = nullptr);

signals:
    void readLogAsync(LogBuffer *logBuffer);
    void readLogResult(bool success, const QString &errorMessage);

public slots:
    void enableIo();
    bool cancelIo();

private slots:
    void readLog(LogBuffer *logBuffer);

private:
    volatile bool m_isWorking;
    volatile bool m_cancelled;

    Device *m_device;

    QMutex m_mutex;
    QWaitCondition m_waitCondition;
};

#endif // DRIVERWORKER_H
