#include "startuputil.h"

#include <QCoreApplication>
#include <QSettings>
#include <QThread>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <fort_version_l.h>

#include <util/fileutil.h>
#include <util/regkey.h>
#include <util/service/servicehandle.h>
#include <util/service/servicemanageriface.h>

namespace {

constexpr RegKey::Root regCurUserRoot = RegKey::HKCU;
const char *const regCurUserRun = R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Run)";

constexpr RegKey::Root regAllUsersRoot = RegKey::HKLM;
const char *const regAllUsersRun = R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Run)";

constexpr RegKey::Root regShellRoot = RegKey::HKCU;
const char *const regShellMenu = R"(SOFTWARE\Classes\SystemFileAssociations\.exe\Shell)";

const wchar_t *const serviceNameStr = L"" APP_BASE_L L"Svc";
const wchar_t *const serviceDisplayStr = L"" APP_NAME_L L" Service";
const wchar_t *const serviceDescriptionStr = L"Manages " APP_NAME_L L" logic as background server";
const wchar_t *const serviceGroupStr = L"NetworkProvider"; // Group of "BFE" service
// Service Dependencies: Double null-terminated array of null-separated names of services
const wchar_t *const serviceDependenciesStr = L"fortfw\0\0";

QString startupShortcutPath()
{
    return FileUtil::applicationsLocation() + QLatin1Char('\\') + "Startup" + QLatin1Char('\\')
            + qApp->applicationName() + ".lnk";
}

QString wrappedAppFilePath()
{
    const auto filePath = FileUtil::toNativeSeparators(FileUtil::nativeAppFilePath());
    return QString("\"%1\"").arg(filePath);
}

bool isAutorunForUser(RegKey::Root root, const char *key)
{
    const RegKey reg(root, key, RegKey::DefaultReadOnly);
    return reg.contains(APP_NAME);
}

bool isAutorunForCurrentUser()
{
    return isAutorunForUser(regCurUserRoot, regCurUserRun);
}

bool isAutorunForAllUsers()
{
    return isAutorunForUser(regAllUsersRoot, regAllUsersRun);
}

void setAutorunForUser(RegKey::Root root, const char *key, const QString &command)
{
    RegKey reg(root, key, RegKey::DefaultReadWrite);
    reg.setValue(APP_NAME, command);
}

void setAutorunForCurrentUser(const QString &command)
{
    setAutorunForUser(regCurUserRoot, regCurUserRun, command);
}

void setAutorunForAllUsers(const QString &command)
{
    setAutorunForUser(regAllUsersRoot, regAllUsersRun, command);
}

void removeAutorunForUser(RegKey::Root root, const char *key)
{
    RegKey reg(root, key, RegKey::DefaultReadWrite);
    reg.removeValue(APP_NAME);
}

void removeAutorunForCurrentUser()
{
    removeAutorunForUser(regCurUserRoot, regCurUserRun);
}

void removeAutorunForAllUsers()
{
    removeAutorunForUser(regAllUsersRoot, regAllUsersRun);
}

bool installService(const CreateServiceArg &csa)
{
    ServiceHandle svc(csa.serviceName, SC_MANAGER_CREATE_SERVICE);
    if (!svc.isManagerOpened())
        return false;

    if (!svc.createService(csa))
        return false;

    return svc.setupServiceRestartConfig();
}

bool uninstallService(const wchar_t *serviceName)
{
    ServiceHandle svc(serviceName, SC_MANAGER_ALL_ACCESS, SERVICE_ALL_ACCESS | DELETE);
    if (svc.isServiceOpened()) {
        svc.stopService();

        return svc.deleteService();
    }

    return false;
}

}

const wchar_t *StartupUtil::serviceName()
{
    return serviceNameStr;
}

bool StartupUtil::isServiceInstalled()
{
    ServiceHandle svc(serviceNameStr, SC_MANAGER_CONNECT, SERVICE_INTERROGATE);

    return svc.isServiceOpened();
}

void StartupUtil::setServiceInstalled(bool install)
{
    if (!install) {
        uninstallService(serviceNameStr);
        return;
    }

    const QString command = wrappedAppFilePath() + " --service";

    const CreateServiceArg csa = {
        .serviceName = serviceNameStr,
        .serviceDisplay = serviceDisplayStr,
        .serviceDescription = serviceDescriptionStr,
        .serviceGroup = serviceGroupStr,
        .dependencies = serviceDependenciesStr,
        .command = (LPCWSTR) command.utf16(),
    };

    installService(csa);

    startService();

    QThread::msleep(100); // Let the service to start
}

bool StartupUtil::isServiceRunning()
{
    ServiceHandle svc(serviceNameStr, SC_MANAGER_CONNECT, SERVICE_INTERROGATE);
    if (svc.isServiceOpened()) {
        return svc.queryIsRunning();
    }

    return false;
}

bool StartupUtil::startService()
{
    ServiceHandle svc(serviceNameStr, SC_MANAGER_CONNECT, SERVICE_START);
    if (svc.isServiceOpened()) {
        return svc.startService();
    }

    return false;
}

bool StartupUtil::stopService()
{
    ServiceHandle svc(serviceNameStr, SC_MANAGER_ALL_ACCESS, SERVICE_ALL_ACCESS);
    if (svc.isServiceOpened()) {
        return svc.stopService();
    }

    return false;
}

StartupUtil::AutoRunMode StartupUtil::autoRunMode()
{
    return isAutorunForCurrentUser() ? StartupCurrentUser
                                     : (isAutorunForAllUsers() ? StartupAllUsers : StartupDisabled);
}

void StartupUtil::setAutoRunMode(int mode, const QString &defaultLanguage)
{
    // Remove link from Programs -> Startup
    // TODO: COMPAT: Remove after v4.1.0 (via v4.0.0)
    FileUtil::removeFile(startupShortcutPath());

    removeAutorunForCurrentUser();
    removeAutorunForAllUsers();

    if (mode == StartupDisabled)
        return;

    const QString command = wrappedAppFilePath()
            + (defaultLanguage.isEmpty() ? QString() : " --lang " + defaultLanguage);

    switch (mode) {
    case StartupCurrentUser:
        setAutorunForCurrentUser(command);
        break;
    case StartupAllUsers:
        setAutorunForAllUsers(command);
        break;
    default:
        Q_UNREACHABLE();
    }
}

bool StartupUtil::isExplorerIntegrated()
{
    const RegKey regShell(regShellRoot, regShellMenu, RegKey::DefaultReadOnly);
    const RegKey reg(regShell, APP_NAME, RegKey::DefaultReadOnly);
    return !reg.isNull();
}

void StartupUtil::setExplorerIntegrated(bool integrate)
{
    RegKey regShell(regShellRoot, regShellMenu,
            integrate ? RegKey::DefaultCreate : RegKey::DefaultReadWrite);

    if (integrate) {
        const QString wrappedPath = wrappedAppFilePath();

        RegKey reg(std::move(regShell), APP_NAME, RegKey::DefaultCreate);
        reg.setValue("icon", wrappedPath);
        reg.setValue("MUIVerb", APP_NAME + QLatin1String(" ..."));

        RegKey regCommand(std::move(reg), "command", RegKey::DefaultCreate);
        regCommand.setDefaultValue(wrappedPath + " -c prog add \"%1\"");
    } else {
        regShell.removeRecursively(APP_NAME);
    }
}

void StartupUtil::clearGlobalExplorerIntegrated()
{
    RegKey regShell(RegKey::HKLM, regShellMenu, RegKey::DefaultReadWrite);

    regShell.removeRecursively(APP_NAME);
}

QString StartupUtil::registryPasswordHash()
{
    const RegKey regApp(RegKey::HKLM, R"(SOFTWARE)");

    const RegKey reg(regApp, APP_NAME);

    return reg.value("passwordHash").toString();
}

void StartupUtil::setRegistryPasswordHash(const QString &passwordHash)
{
    const bool isAdding = !passwordHash.isEmpty();

    const RegKey regApp(RegKey::HKLM, R"(SOFTWARE)",
            isAdding ? RegKey::DefaultCreate : RegKey::DefaultReadWrite);

    RegKey reg(regApp, APP_NAME, RegKey::DefaultCreate);

    if (isAdding) {
        reg.setValue("passwordHash", passwordHash);
    } else {
        reg.removeValue("passwordHash");
    }
}

void StartupUtil::setPortable(bool portable)
{
    const QString readmePortablePath = FileUtil::nativeAppBinLocation() + "/README.portable";

    if (portable) {
        FileUtil::copyFile(":/README.portable", readmePortablePath);
    } else {
        FileUtil::removeFile(readmePortablePath);
    }
}
