#include "logmanager.h"

#include <QLoggingCategory>

#include <conf/confappmanager.h>
#include <conf/confmanager.h>
#include <driver/drivercommon.h>
#include <driver/drivermanager.h>
#include <driver/driverworker.h>
#include <fortglobal.h>
#include <stat/askpendingmanager.h>
#include <stat/statconnmanager.h>
#include <stat/statmanager.h>
#include <util/dateutil.h>
#include <util/osutil.h>
#include <util/processinfo.h>

#include "logbuffer.h"
#include "logentryapp.h"
#include "logentryconn.h"
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
    case FORT_LOG_TYPE_APP:
        return processLogEntryApp(logBuffer);
    case FORT_LOG_TYPE_CONN:
        return processLogEntryConn(logBuffer);
    case FORT_LOG_TYPE_PROC_NEW:
        return processLogEntryProcNew(logBuffer);
    case FORT_LOG_TYPE_STAT_TRAF:
        return processLogEntryStatTraf(logBuffer);
    case FORT_LOG_TYPE_TIME:
        return processLogEntryTime(logBuffer);
    case FORT_LOG_TYPE_PROC_KILL:
        return processLogEntryProcKill(logBuffer);
    default:
        return processLogEntryError(logBuffer, logType);
    }
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
        askPendingManager()->logConn(connEntry);
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
