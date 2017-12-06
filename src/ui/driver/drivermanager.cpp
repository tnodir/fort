#include "drivermanager.h"

#include <QThreadPool>

#include "../conf/firewallconf.h"
#include "../fortcommon.h"
#include "../log/logbuffer.h"
#include "../util/confutil.h"
#include "../util/device.h"
#include "../util/osutil.h"
#include "driverworker.h"

DriverManager::DriverManager(QObject *parent) :
    QObject(parent),
    m_device(new Device(this)),
    m_driverWorker(new DriverWorker(m_device))  // autoDelete = true
{
    setupWorker();
}

DriverManager::~DriverManager()
{
    abortWorker();
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
    QThreadPool::globalInstance()->start(m_driverWorker);
}

void DriverManager::abortWorker()
{
    m_driverWorker->disconnect(this);

    m_driverWorker->abort();
}

bool DriverManager::isDeviceOpened() const
{
    return m_device->isOpened();
}

bool DriverManager::openDevice()
{
    if (!m_device->open(FortCommon::deviceName())) {
        setErrorMessage(OsUtil::lastErrorMessage());
        return false;
    }

    return true;
}

bool DriverManager::closeDevice()
{
    return m_device->close();
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

bool DriverManager::writeData(quint32 code, QByteArray &buf, int size)
{
    m_driverWorker->cancelAsyncIo();

    if (!m_device->ioctl(code, buf.data(), size)) {
        setErrorMessage(OsUtil::lastErrorMessage());
        return false;
    }

    return true;
}
