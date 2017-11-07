#include "driverworker.h"

#include "../fortcommon.h"
#include "../log/logbuffer.h"
#include "../util/device.h"
#include "../util/osutil.h"

DriverWorker::DriverWorker(Device *device, QObject *parent) :
    QObject(parent),
    m_isWorking(false),
    m_cancelled(false),
    m_aborted(false),
    m_device(device),
    m_logBuffer(nullptr)
{
}

void DriverWorker::run()
{
    do {
        if (waitLogBuffer()) {
            readLog();
        }
    } while (!m_aborted);
}

void DriverWorker::readLogAsync(LogBuffer *logBuffer)
{
    QMutexLocker locker(&m_mutex);

    m_cancelled = false;

    m_logBuffer = logBuffer;

    m_waitCondition.wakeOne();
}

void DriverWorker::cancelIo()
{
    QMutexLocker locker(&m_mutex);

    m_cancelled = true;

    m_waitCondition.wakeOne();

    if (m_isWorking) {
        m_device->cancelIo();

        do {
            m_cancelledCondition.wait(&m_mutex);
        } while (m_isWorking);
    }
}

void DriverWorker::abort()
{
    m_aborted = true;

    cancelIo();
}

bool DriverWorker::waitLogBuffer()
{
    QMutexLocker locker(&m_mutex);

    while (!m_logBuffer) {
        if (m_aborted)
            return false;

        m_waitCondition.wait(&m_mutex);
    }

    return true;
}

void DriverWorker::readLog()
{
    // Lock logBuffer
    {
        QMutexLocker locker(&m_mutex);
        if (m_cancelled) {
            return;
        }
        m_isWorking = true;
    }

    QByteArray &array = m_logBuffer->array();
    int nr;

    const bool success = m_device->ioctl(
                FortCommon::ioctlGetLog(), nullptr, 0,
                array.data(), array.size(), &nr);

    QString errorMessage;

    if (success) {
        m_logBuffer->reset(nr);
    } else if (!m_cancelled) {
        errorMessage = OsUtil::lastErrorMessage();
    }

    m_logBuffer = nullptr;

    // Unlock logBuffer
    {
        QMutexLocker locker(&m_mutex);
        m_isWorking = false;
        if (m_cancelled) {
            m_cancelledCondition.wakeOne();
            return;
        }
    }

    emit readLogResult(success, errorMessage);
}
