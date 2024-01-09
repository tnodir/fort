#include "driverworker.h"

#include <driver/drivercommon.h>
#include <log/logbuffer.h>
#include <util/device.h>
#include <util/osutil.h>

DriverWorker::DriverWorker(Device *device, QObject *parent) : QObject(parent), m_device(device) { }

void DriverWorker::run()
{
    OsUtil::setCurrentThreadName("DriverWorker");

    do {
        readLog();
    } while (!m_aborted);
}

bool DriverWorker::readLogAsync(LogBuffer *logBuffer)
{
    QMutexLocker locker(&m_mutex);

    m_cancelled = false;

    bool logBufferUsed = false;
    if (!m_logBuffer) {
        m_logBuffer = logBuffer;
        logBufferUsed = true;
    }

    m_bufferWaitCondition.wakeAll();

    return logBufferUsed;
}

bool DriverWorker::cancelAsyncIo()
{
    QMutexLocker locker(&m_mutex);

    const bool wasCancelled = !m_cancelled;
    m_cancelled = true;

    if (m_isLogReading) {
        m_device->cancelIo();

        do {
            m_cancelledWaitCondition.wait(&m_mutex);
        } while (m_isLogReading);
    }

    return wasCancelled;
}

void DriverWorker::continueAsyncIo()
{
    QMutexLocker locker(&m_mutex);

    m_cancelled = false;

    if (m_logBuffer) {
        m_bufferWaitCondition.wakeAll();
    }
}

void DriverWorker::close()
{
    if (m_aborted)
        return;

    m_aborted = true;

    readLogAsync(nullptr);
}

bool DriverWorker::waitLogBuffer()
{
    QMutexLocker locker(&m_mutex);

    while (!m_logBuffer || m_cancelled) {
        if (m_aborted)
            return false;

        m_bufferWaitCondition.wait(&m_mutex);
    }

    m_isLogReading = true;

    return true;
}

void DriverWorker::emitReadLogResult(bool success, quint32 errorCode)
{
    QMutexLocker locker(&m_mutex);

    m_isLogReading = false;

    LogBuffer *logBuffer = m_logBuffer;
    m_logBuffer = nullptr;

    emit readLogResult(logBuffer, success, errorCode);

    if (m_cancelled) {
        m_cancelledWaitCondition.wakeAll();
    }
}

void DriverWorker::readLog()
{
    if (!waitLogBuffer())
        return;

    QByteArray &array = m_logBuffer->array();
    qsizetype nr = 0;

    const bool success = m_device->ioctl(
            DriverCommon::ioctlGetLog(), nullptr, 0, array.data(), array.size(), &nr);

    quint32 errorCode = 0;

    if (success) {
        m_logBuffer->reset(nr);
    } else if (!m_cancelled) {
        errorCode = OsUtil::lastErrorCode();
    }

    emitReadLogResult(success, errorCode);
}
