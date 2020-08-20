#include "logmanager.h"

#include <QCoreApplication>
#include <QDebug>

#include "../driver/drivermanager.h"
#include "../driver/driverworker.h"
#include "../fortcommon.h"
#include "../fortmanager.h"
#include "../model/applistmodel.h"
#include "../model/appstatmodel.h"
#include "../util/osutil.h"
#include "logbuffer.h"
#include "logentryblocked.h"
#include "logentryprocnew.h"
#include "logentrystattraf.h"

LogManager::LogManager(FortManager *fortManager, QObject *parent) :
    QObject(parent), m_fortManager(fortManager)
{
}

AppListModel *LogManager::appListModel() const
{
    return fortManager()->appListModel();
}

AppStatModel *LogManager::appStatModel() const
{
    return fortManager()->appStatModel();
}

DriverWorker *LogManager::driverWorker() const
{
    return fortManager()->driverManager()->driverWorker();
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
    connect(driverWorker(), &DriverWorker::readLogResult, this, &LogManager::processLogBuffer,
            Qt::QueuedConnection);
}

void LogManager::close()
{
    QCoreApplication::sendPostedEvents(this);

    disconnect(driverWorker());
}

void LogManager::readLogAsync()
{
    LogBuffer *logBuffer = getFreeBuffer();

    if (!driverWorker()->readLogAsync(logBuffer)) {
        addFreeBuffer(logBuffer);
    }
}

void LogManager::cancelAsyncIo()
{
    driverWorker()->cancelAsyncIo();
}

LogBuffer *LogManager::getFreeBuffer()
{
    if (m_freeBuffers.isEmpty())
        return new LogBuffer(FortCommon::bufferSize(), this);

    return m_freeBuffers.takeLast();
}

void LogManager::addFreeBuffer(LogBuffer *logBuffer)
{
    m_freeBuffers.append(logBuffer);
}

void LogManager::processLogBuffer(LogBuffer *logBuffer, bool success, quint32 errorCode)
{
    if (m_active) {
        readLogAsync();
    }

    if (success) {
        readLogEntries(logBuffer);
        logBuffer->reset();
    } else {
        const auto errorMessage = OsUtil::errorMessage(errorCode);
        setErrorMessage(errorMessage);
    }

    addFreeBuffer(logBuffer);
}

void LogManager::readLogEntries(LogBuffer *logBuffer)
{
    for (;;) {
        const auto logType = logBuffer->peekEntryType();

        switch (logType) {
        case LogEntry::AppBlocked: {
            LogEntryBlocked blockedEntry;
            logBuffer->readEntryBlocked(&blockedEntry);
            appListModel()->addLogEntry(blockedEntry);
            break;
        }
        case LogEntry::ProcNew: {
            LogEntryProcNew procNewEntry;
            logBuffer->readEntryProcNew(&procNewEntry);
            appStatModel()->handleProcNew(procNewEntry);
            break;
        }
        case LogEntry::StatTraf: {
            LogEntryStatTraf statTrafEntry;
            logBuffer->readEntryStatTraf(&statTrafEntry);
            appStatModel()->handleStatTraf(statTrafEntry);
            break;
        }
        default:
            if (logBuffer->offset() < logBuffer->top()) {
                qCritical() << "Unknown Log entry!" << logType;
            }
            return;
        }
    }
}
