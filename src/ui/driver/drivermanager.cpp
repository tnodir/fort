#include "drivermanager.h"

#include <QLoggingCategory>
#include <QProcess>
#include <QThreadPool>

#include <conf/firewallconf.h>
#include <driver/drivercommon.h>
#include <util/fileutil.h>
#include <util/osutil.h>

namespace {

const QLoggingCategory LC("driver.manager");

}

DriverManager::DriverManager(QObject *parent, bool useDevice) : QObject(parent)
{
    if (useDevice) {
        setupWorker();
    }
}

DriverManager::~DriverManager()
{
    closeWorker();
}

QString DriverManager::errorMessage() const
{
    return (errorCode() == 0) ? QString() : OsUtil::errorMessage(errorCode());
}

void DriverManager::setErrorCode(quint32 v)
{
    if (m_errorCode != v) {
        m_errorCode = v;
        emit errorCodeChanged();
    }
}

void DriverManager::updateErrorCode(bool success)
{
    setErrorCode(success ? 0 : OsUtil::lastErrorCode());
}

bool DriverManager::isDeviceError() const
{
    return errorCode() != 0 && errorCode() != DriverCommon::userErrorCode();
}

bool DriverManager::isDeviceOpened() const
{
    return device().isOpened();
}

void DriverManager::setUp()
{
    if (driverWorker()) {
        QThreadPool::globalInstance()->start(driverWorker());
    }
}

void DriverManager::setupWorker()
{
    m_driverWorker = new DriverWorker(device()); // autoDelete = true
}

void DriverManager::closeWorker()
{
    if (driverWorker()) {
        driverWorker()->close();
    }
}

bool DriverManager::openDevice()
{
    const bool ok = device().open(DriverCommon::deviceName());

    updateErrorCode(ok);

    emit isDeviceOpenedChanged();

    return ok;
}

bool DriverManager::closeDevice()
{
    const bool ok = device().close();

    updateErrorCode(true);

    emit isDeviceOpenedChanged();

    return ok;
}

bool DriverManager::validate(QByteArray &buf)
{
    return writeData(DriverCommon::ioctlValidate(), buf);
}

bool DriverManager::writeServices(QByteArray &buf)
{
    return writeData(DriverCommon::ioctlSetServices(), buf);
}

bool DriverManager::writeConf(QByteArray &buf, bool onlyFlags)
{
    return writeData(onlyFlags ? DriverCommon::ioctlSetFlags() : DriverCommon::ioctlSetConf(), buf);
}

bool DriverManager::writeApp(QByteArray &buf, bool remove)
{
    return writeData(remove ? DriverCommon::ioctlDelApp() : DriverCommon::ioctlAddApp(), buf);
}

bool DriverManager::writeZones(QByteArray &buf, bool onlyFlags)
{
    const auto code = onlyFlags ? DriverCommon::ioctlSetZoneFlag() : DriverCommon::ioctlSetZones();

    return writeData(code, buf);
}

bool DriverManager::writeRules(QByteArray &buf, bool onlyFlags)
{
    const auto code = onlyFlags ? DriverCommon::ioctlSetRuleFlag() : DriverCommon::ioctlSetRules();

    return writeData(code, buf);
}

bool DriverManager::writeGroups(QByteArray &buf, bool onlyFlags)
{
    const auto code =
            onlyFlags ? DriverCommon::ioctlSetGroupFlag() : DriverCommon::ioctlSetGroups();

    return writeData(code, buf);
}

bool DriverManager::writeData(quint32 code, QByteArray &buf)
{
    if (!isDeviceOpened())
        return true;

    const bool wasCancelled = driverWorker()->cancelAsyncIo();

    const bool ok = device().ioctl(code, buf.data(), buf.size());

    updateErrorCode(ok);

    if (wasCancelled) {
        driverWorker()->continueAsyncIo();
    }

    return ok;
}

bool DriverManager::checkReinstallDriver()
{
    return executeCommand("check-reinstall.bat");
}

bool DriverManager::reinstallDriver()
{
    return executeCommand("reinstall.bat");
}

bool DriverManager::uninstallDriver()
{
    return executeCommand("uninstall.bat");
}

bool DriverManager::executeCommand(const QString &fileName)
{
    const QString binPath = FileUtil::toNativeSeparators(FileUtil::appBinLocation());

    const QString cmdPath = qEnvironmentVariable("COMSPEC");
    const QString scriptPath = binPath + R"(\driver\scripts\execute-cmd.bat)";

    qCDebug(LC) << "executeCommand:" << cmdPath << scriptPath << fileName;

    return QProcess::execute(cmdPath, { "/C", scriptPath, fileName }) == 0;
}
