#include "logmanager.h"

#include <QLoggingCategory>

#include <conf/confappmanager.h>
#include <conf/confmanager.h>
#include <driver/drivercommon.h>
#include <driver/drivermanager.h>
#include <driver/driverworker.h>
#include <fortglobal.h>
#include <stat/pendingconnmanager.h>
#include <stat/statconnmanager.h>
#include <stat/statmanager.h>
#include <util/dateutil.h>
#include <util/osutil.h>
#include <util/processinfo.h>

#include "logbuffer.h"
#include "logentryapp.h"
#include "logentryconn.h"
#include "logentryflowclosed.h"
#include "logentryprockill.h"
#include "logentryprocnew.h"
#include "logentrystattraf.h"
#include "logentrytime.h"

using namespace Fort;

namespace {

const QLoggingCategory LC("log");

DriverWorker *driverWorker()
{
    return driverManager()->driverWorker();
}

/* The log is read or its reading is cancelled without an error */
bool isLogReadOk(bool success, quint32 errorCode)
{
    return success || errorCode == 0;
}

}

LogManager::LogManager(QObject *parent) : QObject(parent) { }

void LogManager::setActive(bool active)
{
    if (m_active == active)
        return;

    m_active = active;

    if (m_active) {
        readLogAsync();
    } else {
        cancelAsyncIo();
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
    connect(driverWorker(), &DriverWorker::readLogResult, this, &LogManager::processLogBuffer,
            Qt::QueuedConnection);
}

void LogManager::tearDown()
{
    driverWorker()->disconnect(this);
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
    if (!m_freeBuffers.isEmpty()) {
        return m_freeBuffers.takeLast();
    }

    return new LogBuffer(DriverCommon::bufferSize(), this);
}

void LogManager::addFreeBuffer(LogBuffer *logBuffer)
{
    constexpr int maxBufferCount = 8;
    if (m_freeBuffers.size() < maxBufferCount) {
        m_freeBuffers.append(logBuffer);
        return;
    }

    delete logBuffer;
}

void LogManager::processLogBuffer(LogBuffer *logBuffer, bool success, quint32 errorCode)
{
    if (m_active && isLogReadOk(success, errorCode)) {
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
    using ProcessFunc = bool (LogManager::*)(LogBuffer *logBuffer);

    static const ProcessFunc processFuncs[] = {
        nullptr, // FORT_LOG_TYPE_NONE
        &LogManager::processLogEntryApp, // FORT_LOG_TYPE_APP
        &LogManager::processLogEntryConn, // FORT_LOG_TYPE_CONN
        nullptr, // FORT_LOG_TYPE_PROMPT
        &LogManager::processLogEntryProcNew, // FORT_LOG_TYPE_PROC_NEW
        &LogManager::processLogEntryStatTraf, // FORT_LOG_TYPE_STAT_TRAF
        &LogManager::processLogEntryTime, // FORT_LOG_TYPE_TIME
        &LogManager::processLogEntryProcKill, // FORT_LOG_TYPE_PROC_KILL
        &LogManager::processLogEntryFlowClosed, // FORT_LOG_TYPE_FLOW_CLOSED
    };
    static_assert(std::size(processFuncs) == FORT_LOG_TYPE_COUNT);

    const auto index = size_t(logType);
    const ProcessFunc func = (index < std::size(processFuncs)) ? processFuncs[index] : nullptr;

    if (!func) {
        return processLogEntryError(logBuffer, logType);
    }

    return (this->*func)(logBuffer);
}

bool LogManager::processLogEntryApp(LogBuffer *logBuffer)
{
    LogEntryApp appEntry;
    logBuffer->readEntryApp(&appEntry);

    confAppManager()->logApp(appEntry);

    return true;
}

bool LogManager::processLogEntryConn(LogBuffer *logBuffer)
{
    LogEntryConn connEntry;
    logBuffer->readEntryConn(&connEntry);

    connEntry.setConnTime(currentUnixTime());

    if (connEntry.isAskPending()) {
        pendingConnManager()->logConn(connEntry, /*urgent=*/true);
    } else {
        statConnManager()->logConn(connEntry);

        if (connEntry.alerted() && connEntry.appId() != 0) {
            confAppManager()->addAlertedApp(connEntry.appId());
        }
    }

    return true;
}

bool LogManager::processLogEntryProcNew(LogBuffer *logBuffer)
{
    LogEntryProcNew procNewEntry;
    logBuffer->readEntryProcNew(&procNewEntry);

    statManager()->logProcNew(procNewEntry, currentUnixTime());

    return true;
}

bool LogManager::processLogEntryStatTraf(LogBuffer *logBuffer)
{
    LogEntryStatTraf statTrafEntry;
    logBuffer->readEntryStatTraf(&statTrafEntry);

    statManager()->logStatTraf(statTrafEntry, currentUnixTime());

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

bool LogManager::processLogEntryProcKill(LogBuffer *logBuffer)
{
    LogEntryProcKill procKillEntry;
    logBuffer->readEntryProcKill(&procKillEntry);

    ProcessInfo pi(procKillEntry.pid(), ProcessInfo::OpenTerminate);
    if (pi.isValid()) {
        pi.terminateProcess(procKillEntry.pid());
    }

    return true;
}

bool LogManager::processLogEntryFlowClosed(LogBuffer *logBuffer)
{
    // Not used yet
    LogEntryFlowClosed flowClosedEntry;
    logBuffer->readEntryFlowClosed(&flowClosedEntry);

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
