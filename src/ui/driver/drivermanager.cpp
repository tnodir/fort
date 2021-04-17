#include "drivermanager.h"

#include <QProcess>
#include <QThreadPool>

#include "../conf/firewallconf.h"
#include "../driver/drivercommon.h"
#include "../util/device.h"
#include "../util/fileutil.h"
#include "../util/osutil.h"
#include "driverworker.h"

DriverManager::DriverManager(QObject *parent) :
    QObject(parent),
    m_device(new Device(this)),
    m_driverWorker(new DriverWorker(m_device)) // autoDelete = true
{
    setupWorker();
}

DriverManager::~DriverManager()
{
    abortWorker();
}

QString DriverManager::errorMessage() const
{
    return (m_errorCode == 0) ? QString() : OsUtil::errorMessage(m_errorCode);
}

void DriverManager::updateError(bool success)
{
    m_errorCode = success ? 0 : OsUtil::lastErrorCode();
    emit errorMessageChanged();
}

bool DriverManager::isDeviceError() const
{
    return m_errorCode != 0 && m_errorCode != DriverCommon::userErrorCode();
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
    const bool res = m_device->open(DriverCommon::deviceName());

    updateError(res);

    emit isDeviceOpenedChanged();

    return res;
}

bool DriverManager::closeDevice()
{
    const bool res = m_device->close();

    emit isDeviceOpenedChanged();

    return res;
}

bool DriverManager::validate(QByteArray &buf, int size)
{
    return writeData(DriverCommon::ioctlValidate(), buf, size);
}

bool DriverManager::writeConf(QByteArray &buf, int size, bool onlyFlags)
{
    return writeData(
            onlyFlags ? DriverCommon::ioctlSetFlags() : DriverCommon::ioctlSetConf(), buf, size);
}

bool DriverManager::writeApp(QByteArray &buf, int size, bool remove)
{
    return writeData(remove ? DriverCommon::ioctlDelApp() : DriverCommon::ioctlAddApp(), buf, size);
}

bool DriverManager::writeZones(QByteArray &buf, int size, bool onlyFlags)
{
    return writeData(onlyFlags ? DriverCommon::ioctlSetZoneFlag() : DriverCommon::ioctlSetZones(),
            buf, size);
}

bool DriverManager::writeData(quint32 code, QByteArray &buf, int size)
{
    if (!isDeviceOpened())
        return true;

    m_driverWorker->cancelAsyncIo();

    const bool res = m_device->ioctl(code, buf.data(), size);

    updateError(res);

    return res;
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
    const QString binPath = FileUtil::toNativeSeparators(FileUtil::appBinLocation());

    const QString cmdPath = qEnvironmentVariable("COMSPEC");
    const QString scriptPath = binPath + R"(\driver\scripts\execute-cmd.bat)";

    QProcess::execute(cmdPath, QStringList() << "/C" << scriptPath << fileName);
}
