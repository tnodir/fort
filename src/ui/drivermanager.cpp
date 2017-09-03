#include "drivermanager.h"

#include "fortcommon.h"
#include "conf/firewallconf.h"
#include "util/confutil.h"
#include "util/device.h"

DriverManager::DriverManager(QObject *parent) :
    QObject(parent),
    m_device(new Device(this))
{
}

void DriverManager::setErrorMessage(const QString &errorMessage)
{
    if (m_errorMessage != errorMessage) {
        m_errorMessage = errorMessage;
        emit errorMessageChanged();
    }
}

bool DriverManager::openDevice()
{
    if (!m_device->open(FortCommon::deviceName())) {
        setErrorMessage(m_device->getLastErrorMessage());
        return false;
    }

    return true;
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
    if (!m_device->ioctl(code, buf.data(), size)) {
        setErrorMessage(m_device->getLastErrorMessage());
        return false;
    }

    return true;
}
