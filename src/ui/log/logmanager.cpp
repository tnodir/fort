#include "logmanager.h"

#include "../driver/driverworker.h"
#include "../fortcommon.h"
#include "db/databasemanager.h"
#include "logbuffer.h"
#include "logentryblocked.h"
#include "logentryprocnew.h"
#include "logentrystattraf.h"
#include "model/appblockedmodel.h"

LogManager::LogManager(DatabaseManager *databaseManager,
                       DriverWorker *driverWorker,
                       QObject *parent) :
    QObject(parent),
    m_logReadingEnabled(false),
    m_databaseManager(databaseManager),
    m_driverWorker(driverWorker),
    m_appBlockedModel(new AppBlockedModel(this))
{
    setupDriverWorker();
}

void LogManager::clearModels() const
{
    m_appBlockedModel->clear();
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
            readLogAsync(getFreeBuffer());
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
    if (m_logReadingEnabled) {
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
            logBuffer->readEntryBlocked(&entryBlocked);
            m_appBlockedModel->addLogEntry(entryBlocked);
            break;
        }
        case LogEntry::ProcNew: {
            logBuffer->readEntryProcNew(&entryProcNew);
            m_databaseManager->handleProcNew(entryProcNew.path());
            break;
        }
        case LogEntry::StatTraf: {
            logBuffer->readEntryStatTraf(&entryStatTraf);
            m_databaseManager->handleStatTraf(
                        entryStatTraf.procCount(), entryStatTraf.procBits(),
                        entryStatTraf.trafBytes());
            break;
        }
        default:
            return;
        }
    }
}
