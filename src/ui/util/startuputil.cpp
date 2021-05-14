#include "startuputil.h"

#include <QCoreApplication>
#include <QSettings>
#include <QThread>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <fort_version.h>

#include "fileutil.h"
#include "regkey.h"

namespace {

constexpr RegKey::Root regCurUserRoot = RegKey::HKCU;
const char *const regCurUserRun = R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Run)";

constexpr RegKey::Root regAllUsersRoot = RegKey::HKLM;
const char *const regAllUsersRun = R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Run)";

constexpr RegKey::Root regShellRoot = RegKey::HKLM;
const char *const regShellMenu = R"(SOFTWARE\Classes\SystemFileAssociations\.exe\Shell)";

QString startupShortcutPath()
{
    return FileUtil::applicationsLocation() + QLatin1Char('\\') + "Startup" + QLatin1Char('\\')
            + qApp->applicationName() + ".lnk";
}

QString wrappedAppFilePath()
{
    const auto filePath = FileUtil::toNativeSeparators(QCoreApplication::applicationFilePath());
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

bool installService(
        const wchar_t *serviceName, const wchar_t *serviceDisplay, const QString &command)
{
    bool res = false;
    const SC_HANDLE mngr = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (mngr) {
        const SC_HANDLE svc = CreateServiceW(mngr, serviceName, serviceDisplay, SERVICE_ALL_ACCESS,
                SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
                (LPCWSTR) command.utf16(), 0, 0, 0, 0, 0);
        if (svc) {
            res = true;
            CloseServiceHandle(svc);
        }
        CloseServiceHandle(mngr);
    }
    return res;
}

bool uninstallService(const wchar_t *serviceName)
{
    bool res = false;
    const SC_HANDLE mngr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (mngr) {
        const SC_HANDLE svc = OpenServiceW(mngr, serviceName, SERVICE_ALL_ACCESS | DELETE);
        if (svc) {
            int n = 2; /* count of attempts to stop the service */
            do {
                SERVICE_STATUS status;
                if (QueryServiceStatus(svc, &status) && status.dwCurrentState == SERVICE_STOPPED)
                    break;
                ControlService(svc, SERVICE_CONTROL_STOP, &status);
                QThread::msleep(1000);
            } while (--n > 0);
            res = DeleteService(svc);
            CloseServiceHandle(svc);
        }
        CloseServiceHandle(mngr);
    }
    return res;
}

}

const wchar_t *const StartupUtil::serviceName = L"" APP_BASE "Svc";
const wchar_t *const StartupUtil::serviceDisplay = L"" APP_NAME " Service";

bool StartupUtil::isServiceInstalled()
{
    bool res = false;
    const SC_HANDLE mngr = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (mngr) {
        const SC_HANDLE svc = OpenServiceW(mngr, serviceName, SERVICE_INTERROGATE);
        if (svc) {
            res = true;
            CloseServiceHandle(svc);
        }
        CloseServiceHandle(mngr);
    }
    return res;
}

bool StartupUtil::startService()
{
    bool res = false;
    const SC_HANDLE mngr = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (mngr) {
        const SC_HANDLE svc = OpenServiceW(mngr, serviceName, SERVICE_ALL_ACCESS);
        if (svc) {
            res = StartServiceW(svc, 0, nullptr);
            CloseServiceHandle(svc);
        }
        CloseServiceHandle(mngr);
    }
    return res;
}

StartupUtil::StartupMode StartupUtil::getStartupMode()
{
    const bool isForAllUsers = isAutorunForAllUsers();
    return (isForAllUsers || isServiceInstalled())
            ? (isForAllUsers ? StartupAllUsers : StartupAllUsersBackground)
            : (isAutorunForCurrentUser() ? StartupCurrentUser : StartupDisabled);
}

void StartupUtil::setStartupMode(int mode, const QString &defaultLanguage)
{
    // COMPAT: Remove link from Programs -> Startup
    // TODO: Remove after v4.1.0 (via v4.0.0)
    FileUtil::removeFile(startupShortcutPath());

    removeAutorunForCurrentUser();
    removeAutorunForAllUsers();
    uninstallService(serviceName);

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
        Q_FALLTHROUGH();
    case StartupAllUsersBackground:
        installService(serviceName, serviceDisplay, command + " --service");
        break;
    }
}

bool StartupUtil::isServiceMode(int mode)
{
    return mode == StartupAllUsers || mode == StartupAllUsersBackground;
}

bool StartupUtil::isExplorerIntegrated()
{
    const RegKey regShell(regShellRoot, regShellMenu, RegKey::DefaultReadOnly);
    const RegKey reg(regShell, APP_NAME, RegKey::DefaultReadOnly);
    return !reg.isNull();
}

void StartupUtil::integrateExplorer(bool integrate)
{
    RegKey regShell(regShellRoot, regShellMenu, RegKey::DefaultReadWrite);
    if (integrate) {
        const QString wrappedPath = wrappedAppFilePath();

        RegKey reg(regShell, APP_NAME, RegKey::DefaultCreate);
        reg.setValue("icon", wrappedPath);
        reg.setValue("MUIVerb", APP_NAME + QLatin1String(" ..."));

        RegKey regCommand(reg, "command", RegKey::DefaultCreate);
        regCommand.setDefaultValue(wrappedPath + " -w -c prog add \"%1\"");
    } else {
        regShell.removeRecursively(APP_NAME);
    }
}
