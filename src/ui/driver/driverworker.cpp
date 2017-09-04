#include "driverworker.h"

#include "../activityLog/logbuffer.h"
#include "../fortcommon.h"
#include "../util/device.h"

DriverWorker::DriverWorker(Device *device, QObject *parent) :
    QObject(parent),
    m_isWorking(false),
    m_cancelled(false),
    m_device(device)
{
    connect(this, &DriverWorker::readLogAsync,
            this, &DriverWorker::readLog, Qt::QueuedConnection);
}

void DriverWorker::enableIo()
{
    m_mutex.lock();
    m_cancelled = false;
    m_mutex.unlock();
}

bool DriverWorker::cancelIo()
{
    m_cancelled = true;

    const bool res = m_isWorking ? m_device->cancelIo() : false;

    m_mutex.lock();
    while (m_isWorking) {
        m_waitCondition.wait(&m_mutex);
    }
    m_mutex.unlock();

    return res;
}

void DriverWorker::readLog(LogBuffer *logBuffer)
{
    QByteArray &array = logBuffer->array();
    int nr;

    bool success;
    QString errorMessage;

    m_mutex.lock();
    if (!m_cancelled) {
        m_isWorking = true;
        success = m_device->ioctl(
                    FortCommon::ioctlGetLog(), nullptr, 0,
                    array.data(), array.size(), &nr);
        m_isWorking = false;

        errorMessage = m_cancelled
                ? QString() : m_device->lastErrorMessage();

        if (m_cancelled) {
            m_waitCondition.wakeOne();
        }
    }
    m_mutex.unlock();

    if (success) {
        logBuffer->reset(nr);
    }

    emit readLogResult(success, errorMessage);
}
