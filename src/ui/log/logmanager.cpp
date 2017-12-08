#include "logmanager.h"

#include "../driver/driverworker.h"
#include "../fortcommon.h"
#include "logbuffer.h"
#include "logentryblocked.h"
#include "logentryprocnew.h"
#include "logentrystattraf.h"
#include "model/appblockedmodel.h"
#include "model/appstatmodel.h"

LogManager::LogManager(DatabaseManager *databaseManager,
                       DriverWorker *driverWorker,
                       QObject *parent) :
    QObject(parent),
    m_active(false),
    m_logBlockedEnabled(false),
    m_logStatEnabled(false),
    m_driverWorker(driverWorker),
    m_appBlockedModel(new AppBlockedModel(this)),
    m_appStatModel(new AppStatModel(databaseManager, this))
{
    setupDriverWorker();
}

void LogManager::setActive(bool active)
{
    if (m_active != active) {
        m_active = active;

        if (m_active) {
            readLogAsync(getFreeBuffer());
        } else {
            cancelAsyncIo();
            readLogAsync(nullptr);
        }

        emit activeChanged();
    }
}

void LogManager::setLogBlockedEnabled(bool enabled)
{
    if (m_logBlockedEnabled != enabled) {
        m_logBlockedEnabled = enabled;
        emit logBlockedEnabledChanged();
    }
}

void LogManager::setLogStatEnabled(bool enabled)
{
    if (m_logStatEnabled != enabled) {
        m_logStatEnabled = enabled;
        emit logStatEnabledChanged();
    }
}

void LogManager::setErrorMessage(const QString &errorMessage)
{
    if (m_errorMessage != errorMessage) {
        m_errorMessage = errorMessage;
        emit errorMessageChanged();
    }
}

void LogManager::initialize()
{
    m_appStatModel->initialize();
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

LogBuffer *LogManager::getFreeBuffer()
{
    if (m_freeBuffers.isEmpty()) {
        return new LogBuffer(FortCommon::bufferSize(), this);
    } else {
        return m_freeBuffers.takeLast();
    }
}

void LogManager::processLogBuffer(LogBuffer *logBuffer, bool success,
                                  const QString &errorMessage)
{
    if (m_active) {
        readLogAsync(getFreeBuffer());
    }

    if (success) {
        readLogEntries(logBuffer);
        logBuffer->reset();
    } else {
        setErrorMessage(errorMessage);
    }

    m_freeBuffers.append(logBuffer);
}

void LogManager::readLogEntries(LogBuffer *logBuffer)
{
    LogEntryBlocked entryBlocked;
    LogEntryProcNew entryProcNew;
    LogEntryStatTraf entryStatTraf;

    forever {
        switch (logBuffer->peekEntryType()) {
        case LogEntry::AppBlocked: {
            if (m_logBlockedEnabled) {
                logBuffer->readEntryBlocked(&entryBlocked);
                m_appBlockedModel->addLogEntry(entryBlocked);
            }
            break;
        }
        case LogEntry::ProcNew: {
            if (m_logStatEnabled) {
                logBuffer->readEntryProcNew(&entryProcNew);
                m_appStatModel->handleProcNew(entryProcNew.path());
            }
            break;
        }
        case LogEntry::StatTraf: {
            if (m_logStatEnabled) {
                logBuffer->readEntryStatTraf(&entryStatTraf);
                m_appStatModel->handleStatTraf(
                            entryStatTraf.procCount(),
                            entryStatTraf.procBits(),
                            entryStatTraf.trafBytes());
            }
            break;
        }
        default:
            return;
        }
    }
}
