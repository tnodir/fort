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
            readLogAsync();
        } else {
            cancelAsyncIo();
        }

        emit activeChanged();
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

void LogManager::readLogAsync()
{
    LogBuffer *logBuffer = getFreeBuffer();

    if (!m_driverWorker->readLogAsync(logBuffer)) {
        addFreeBuffer(logBuffer);
    }
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

void LogManager::addFreeBuffer(LogBuffer *logBuffer)
{
    m_freeBuffers.append(logBuffer);
}

void LogManager::processLogBuffer(LogBuffer *logBuffer, bool success,
                                  const QString &errorMessage)
{
    if (m_active) {
        readLogAsync();
    }

    if (success) {
        readLogEntries(logBuffer);
        logBuffer->reset();
    } else {
        setErrorMessage(errorMessage);
    }

    addFreeBuffer(logBuffer);
}

void LogManager::readLogEntries(LogBuffer *logBuffer)
{
    LogEntryBlocked blockedEntry;
    LogEntryProcNew procNewEntry;
    LogEntryStatTraf statTrafEntry;

    forever {
        switch (logBuffer->peekEntryType()) {
        case LogEntry::AppBlocked: {
            logBuffer->readEntryBlocked(&blockedEntry);
            m_appBlockedModel->addLogEntry(blockedEntry);
            break;
        }
        case LogEntry::ProcNew: {
            logBuffer->readEntryProcNew(&procNewEntry);
            m_appStatModel->handleProcNew(procNewEntry);
            break;
        }
        case LogEntry::StatTraf: {
            logBuffer->readEntryStatTraf(&statTrafEntry);
            m_appStatModel->handleStatTraf(statTrafEntry);
            break;
        }
        default:
            Q_ASSERT(logBuffer->offset() != 0);
            return;
        }
    }
}
