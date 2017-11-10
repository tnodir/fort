#include "logmanager.h"

#include "../driver/driverworker.h"
#include "../fortcommon.h"
#include "logbuffer.h"

LogManager::LogManager(DriverWorker *driverWorker,
                       QObject *parent) :
    QObject(parent),
    m_logReadingEnabled(false),
    m_driverWorker(driverWorker),
    m_logBuffer(new LogBuffer(FortCommon::bufferSize(), this))
{
    setupDriverWorker();
}

void LogManager::setErrorMessage(const QString &errorMessage)
{
    if (m_errorMessage != errorMessage) {
        m_errorMessage = errorMessage;
        emit errorMessageChanged();
    }
}

void LogManager::setLogReadingEnabled(bool enabled)
{
    if (m_logReadingEnabled != enabled) {
        m_logReadingEnabled = enabled;

        if (m_logReadingEnabled) {
            readLogAsync(m_logBuffer);
        } else {
            cancelAsyncIo();
            readLogAsync(nullptr);
        }
    }
}

void LogManager::setupDriverWorker()
{
    connect(m_driverWorker, &DriverWorker::readLogResult,
            this, &LogManager::processLogBuffer, Qt::QueuedConnection);
}

void LogManager::readLogAsync(LogBuffer *logBuffer)
{
    m_driverWorker->readLogAsync(logBuffer);
}

void LogManager::cancelAsyncIo()
{
    m_driverWorker->cancelAsyncIo();
}

void LogManager::processLogBuffer(LogBuffer *logBuffer, bool success,
                                  const QString &errorMessage)
{
    Q_ASSERT(logBuffer == m_logBuffer);

    if (success) {
    } else {
        setErrorMessage(errorMessage);
    }

    if (m_logReadingEnabled) {
        readLogAsync(m_logBuffer);
    }
}
