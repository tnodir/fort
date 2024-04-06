#include "logmanager.h"

#include <QLoggingCategory>

#include <conf/confappmanager.h>
#include <conf/confmanager.h>
#include <driver/drivercommon.h>
#include <driver/drivermanager.h>
#include <driver/driverworker.h>
#include <stat/askpendingmanager.h>
#include <stat/statblockmanager.h>
#include <stat/statmanager.h>
#include <util/dateutil.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>

#include "logbuffer.h"
#include "logentryblocked.h"
#include "logentryblockedip.h"
#include "logentryprocnew.h"
#include "logentrystattraf.h"
#include "logentrytime.h"

namespace {
const QLoggingCategory LC("log");
}

LogManager::LogManager(QObject *parent) : QObject(parent) { }

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

qint64 LogManager::currentUnixTime() const
{
    return m_currentUnixTime != 0 ? m_currentUnixTime : DateUtil::getUnixTime();
}

void LogManager::setCurrentUnixTime(qint64 unixTime)
{
    m_currentUnixTime = unixTime;
}

void LogManager::setUp()
{
    const auto driverManager = IoCDependency<DriverManager>();

    connect(driverManager->driverWorker(), &DriverWorker::readLogResult, this,
            &LogManager::processLogBuffer, Qt::QueuedConnection);
}

void LogManager::tearDown()
{
    const auto driverManager = IoC<DriverManager>();

    disconnect(driverManager->driverWorker());
}

void LogManager::readLogAsync()
{
    const auto driverManager = IoC<DriverManager>();

    LogBuffer *logBuffer = getFreeBuffer();

    if (!driverManager->driverWorker()->readLogAsync(logBuffer)) {
        addFreeBuffer(logBuffer);
    }
}

void LogManager::cancelAsyncIo()
{
    const auto driverManager = IoC<DriverManager>();

    driverManager->driverWorker()->cancelAsyncIo();
}

LogBuffer *LogManager::getFreeBuffer()
{
    if (m_freeBuffers.isEmpty())
        return new LogBuffer(DriverCommon::bufferSize(), this);

    return m_freeBuffers.takeLast();
}

void LogManager::addFreeBuffer(LogBuffer *logBuffer)
{
    m_freeBuffers.append(logBuffer);
}

void LogManager::processLogBuffer(LogBuffer *logBuffer, bool success, quint32 errorCode)
{
    if (m_active && (success || errorCode == 0)) {
        readLogAsync();
    }

    if (success) {
        processLogEntries(logBuffer);
    } else if (errorCode != 0) {
        const auto errorMessage = OsUtil::errorMessage(errorCode);
        setErrorMessage(errorMessage);
    }

    logBuffer->reset();
    addFreeBuffer(logBuffer);
}

void LogManager::processLogEntries(LogBuffer *logBuffer)
{
    // XXX: OsUtil::setThreadIsBusy(true);

    for (;;) {
        const FortLogType logType = logBuffer->peekEntryType();

        if (!processLogEntry(logBuffer, logType))
            break;
    }

    // XXX: OsUtil::setThreadIsBusy(false);
}

bool LogManager::processLogEntry(LogBuffer *logBuffer, FortLogType logType)
{
    switch (logType) {
    case FORT_LOG_TYPE_BLOCKED:
    case FORT_LOG_TYPE_ALLOWED:
        return processLogEntryBlocked(logBuffer);
    case FORT_LOG_TYPE_BLOCKED_IP:
        return processLogEntryBlockedIp(logBuffer);
    case FORT_LOG_TYPE_PROC_NEW:
        return processLogEntryProcNew(logBuffer);
    case FORT_LOG_TYPE_STAT_TRAF:
        return processLogEntryStatTraf(logBuffer);
    case FORT_LOG_TYPE_TIME:
        return processLogEntryTime(logBuffer);
    default:
        return processLogEntryError(logBuffer, logType);
    }
}

bool LogManager::processLogEntryBlocked(LogBuffer *logBuffer)
{
    LogEntryBlocked blockedEntry;
    logBuffer->readEntryBlocked(&blockedEntry);

    IoC<ConfAppManager>()->logBlockedApp(blockedEntry);

    return true;
}

bool LogManager::processLogEntryBlockedIp(LogBuffer *logBuffer)
{
    LogEntryBlockedIp blockedIpEntry;
    logBuffer->readEntryBlockedIp(&blockedIpEntry);

    blockedIpEntry.setConnTime(currentUnixTime());

    if (blockedIpEntry.isAskPending()) {
        IoC<AskPendingManager>()->logBlockedIp(blockedIpEntry);
    } else {
        IoC<StatBlockManager>()->logBlockedIp(blockedIpEntry);
    }

    return true;
}

bool LogManager::processLogEntryProcNew(LogBuffer *logBuffer)
{
    LogEntryProcNew procNewEntry;
    logBuffer->readEntryProcNew(&procNewEntry);

    IoC<StatManager>()->logProcNew(procNewEntry, currentUnixTime());

    return true;
}

bool LogManager::processLogEntryStatTraf(LogBuffer *logBuffer)
{
    LogEntryStatTraf statTrafEntry;
    logBuffer->readEntryStatTraf(&statTrafEntry);

    IoC<StatManager>()->logStatTraf(statTrafEntry, currentUnixTime());

    return true;
}

bool LogManager::processLogEntryTime(LogBuffer *logBuffer)
{
    LogEntryTime timeEntry;
    logBuffer->readEntryTime(&timeEntry);

    setCurrentUnixTime(timeEntry.unixTime());

    if (timeEntry.systemTimeChanged()) {
        emit systemTimeChanged();
    }

    return true;
}

bool LogManager::processLogEntryError(LogBuffer *logBuffer, FortLogType logType)
{
    if (logBuffer->offset() < logBuffer->top()) {
        const auto data =
                QByteArray::fromRawData(logBuffer->array().constData() + logBuffer->offset(),
                        logBuffer->top() - logBuffer->offset());

        qCCritical(LC) << "Unknown Log entry:" << logType << logBuffer->offset() << logBuffer->top()
                       << data;
    }

    return false;
}
