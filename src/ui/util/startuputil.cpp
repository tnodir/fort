#include "startuputil.h"

#include <QCoreApplication>
#include <QSettings>
#include <QThread>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <fort_version.h>

#include "fileutil.h"

namespace {

const char *const regCurUserRun =
        R"(HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run)";
const char *const regAllUsersRun =
        R"(HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Run)";

QString startupShortcutPath()
{
    return FileUtil::applicationsLocation() + QLatin1Char('\\') + "Startup" + QLatin1Char('\\')
            + qApp->applicationName() + ".lnk";
}

QString wrappedAppFilePath()
{
    const auto filePath = QCoreApplication::applicationFilePath().replace('/', '\\');
    return QString("\"%1\"").arg(filePath);
}

bool isAutorunForUser(const char *key)
{
    const QSettings reg(key, QSettings::Registry64Format);
    return !reg.value(APP_NAME).isNull();
}

bool isAutorunForCurrentUser()
{
    return isAutorunForUser(regCurUserRun);
}

bool isAutorunForAllUsers()
{
    return isAutorunForUser(regAllUsersRun);
}

void setAutorunForUser(const char *key, const QString &command)
{
    QSettings reg(key, QSettings::Registry64Format);
    reg.setValue(APP_NAME, command);
}

void setAutorunForCurrentUser(const QString &command)
{
    setAutorunForUser(regCurUserRun, command);
}

void setAutorunForAllUsers(const QString &command)
{
    setAutorunForUser(regAllUsersRun, command);
}

void removeAutorunForUser(const char *key)
{
    QSettings reg(key, QSettings::Registry64Format);
    reg.remove(APP_NAME);
}

void removeAutorunForCurrentUser()
{
    removeAutorunForUser(regCurUserRun);
}

void removeAutorunForAllUsers()
{
    removeAutorunForUser(regAllUsersRun);
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
const wchar_t *const StartupUtil::serviceDisplay = L"" APP_NAME;

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
    return isServiceInstalled()
            ? (isAutorunForAllUsers() ? StartupAllUsers : StartupAllUsersBackground)
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
