#include "drivermanager.h"

#include <QThread>

#include "../activityLog/logbuffer.h"
#include "../activityLog/logentry.h"
#include "../fortcommon.h"
#include "../conf/firewallconf.h"
#include "../util/confutil.h"
#include "../util/device.h"
#include "driverworker.h"

DriverManager::DriverManager(QObject *parent) :
    QObject(parent),
    m_device(new Device(this)),
    m_driverWorker(new DriverWorker(m_device)),  // no parent, delete later
    m_workerThread(new QThread(this))
{
    setupWorker();
}

DriverManager::~DriverManager()
{
    cancelDeviceIo();

    m_workerThread->quit();
    m_workerThread->wait();
}

void DriverManager::setErrorMessage(const QString &errorMessage)
{
    if (m_errorMessage != errorMessage) {
        m_errorMessage = errorMessage;
        emit errorMessageChanged();
    }
}

void DriverManager::setupWorker()
{
    m_driverWorker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::finished,
            m_driverWorker, &QObject::deleteLater);

    connect(m_driverWorker, &DriverWorker::readLogResult,
            this, &DriverManager::readLogResult);

    m_workerThread->start();
}

bool DriverManager::isDeviceOpened() const
{
    return m_device->isOpened();
}

bool DriverManager::openDevice()
{
    if (!m_device->open(FortCommon::deviceName())) {
        setErrorMessage(m_device->lastErrorMessage());
        return false;
    }

    return true;
}

void DriverManager::enableDeviceIo()
{
    m_driverWorker->enableIo();
}

bool DriverManager::cancelDeviceIo()
{
    return m_driverWorker->cancelIo();
}

bool DriverManager::writeConf(const FirewallConf &conf)
{
    ConfUtil confUtil;
    QByteArray buf;

    const int confSize = confUtil.write(conf, buf);
    if (!confSize) {
        setErrorMessage(confUtil.errorMessage());
        return false;
    }

    return writeData(FortCommon::ioctlSetConf(),
                     buf, confSize);
}

bool DriverManager::writeConfFlags(const FirewallConf &conf)
{
    ConfUtil confUtil;
    QByteArray buf;

    const int flagsSize = confUtil.writeFlags(conf, buf);
    if (!flagsSize) {
        setErrorMessage(confUtil.errorMessage());
        return false;
    }

    return writeData(FortCommon::ioctlSetFlags(),
                     buf, flagsSize);
}

bool DriverManager::writeData(int code, QByteArray &buf, int size)
{
    cancelDeviceIo();

    if (!m_device->ioctl(code, buf.data(), size)) {
        setErrorMessage(m_device->lastErrorMessage());
        return false;
    }

    return true;
}

void DriverManager::readLogAsync(LogBuffer *logBuffer)
{
    cancelDeviceIo();
    enableDeviceIo();

    emit m_driverWorker->readLogAsync(logBuffer);
}
