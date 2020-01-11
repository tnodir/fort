#include "drivermanager.h"

#include <QProcess>
#include <QThreadPool>

#include "../conf/confmanager.h"
#include "../conf/firewallconf.h"
#include "../fortcommon.h"
#include "../log/logbuffer.h"
#include "../util/conf/confutil.h"
#include "../util/device.h"
#include "../util/fileutil.h"
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
    m_driverWorker->abort();
}

bool DriverManager::isDeviceOpened() const
{
    return m_device->isOpened();
}

bool DriverManager::openDevice()
{
    const bool res = m_device->open(FortCommon::deviceName());

    setErrorMessage(res ? QString() : OsUtil::lastErrorMessage());

    emit isDeviceOpenedChanged();

    return res;
}

bool DriverManager::closeDevice()
{
    const bool res = m_device->close();

    emit isDeviceOpenedChanged();

    return res;
}

bool DriverManager::validate()
{
    ConfUtil confUtil;
    QByteArray buf;

    const int verSize = confUtil.writeVersion(buf);
    if (!verSize) {
        setErrorMessage(confUtil.errorMessage());
        return false;
    }

    return writeData(FortCommon::ioctlValidate(),
                     buf, verSize);
}

bool DriverManager::writeConf(const FirewallConf &conf,
                              ConfManager &confManager,
                              EnvManager &envManager)
{
    if (!isDeviceOpened())
        return true;

    ConfUtil confUtil;
    QByteArray buf;

    const int confSize = confUtil.write(conf, &confManager, envManager, buf);
    if (confSize == 0) {
        setErrorMessage(confUtil.errorMessage());
        return false;
    }

    return writeData(FortCommon::ioctlSetConf(),
                     buf, confSize);
}

bool DriverManager::writeConfFlags(const FirewallConf &conf)
{
    if (!isDeviceOpened())
        return true;

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

bool DriverManager::writeApp(const QString &appPath,
                             int groupIndex, bool useGroupPerm,
                             bool blocked, bool alerted, bool remove)
{
    if (!isDeviceOpened())
        return true;

    ConfUtil confUtil;
    QByteArray buf;

    const int entrySize = confUtil.writeAppEntry(
                groupIndex, useGroupPerm, blocked, alerted,
                appPath, buf);

    if (entrySize == 0) {
        setErrorMessage(confUtil.errorMessage());
        return false;
    }

    return writeData(remove ? FortCommon::ioctlDelApp()
                            : FortCommon::ioctlAddApp(),
                     buf, entrySize);
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

void DriverManager::reinstallDriver()
{
    executeCommand("reinstall.lnk");
}

void DriverManager::uninstallDriver()
{
    executeCommand("uninstall.lnk");
}

void DriverManager::executeCommand(const QString &fileName)
{
    const QString binPath = FileUtil::toNativeSeparators(
                FileUtil::appBinLocation());

    const QString cmdPath = qEnvironmentVariable("COMSPEC");
    const QString scriptPath = binPath + R"(\driver\scripts\execute-cmd.bat)";

    QProcess::execute(cmdPath, QStringList() << "/C" << scriptPath << fileName);
}
