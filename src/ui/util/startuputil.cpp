#include "startuputil.h"

#include <QCoreApplication>
#include <QSettings>
#include <QThread>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <fort_version.h>

#include "../service/servicemanager.h"

namespace {

const char *const serviceName = "FortFirewallSvc";

const char *const regCurUserRun =
        R"(HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run)";
const char *const regAllUsersRun =
        R"(HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Run)";

QString appFilePath()
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

void setAutorunForUser(const char *key)
{
    QSettings reg(key, QSettings::Registry64Format);
    reg.setValue(APP_NAME, appFilePath());
}

void setAutorunForCurrentUser()
{
    setAutorunForUser(regCurUserRun);
}

void setAutorunForAllUsers()
{
    setAutorunForUser(regAllUsersRun);
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

}

bool StartupUtil::isServiceInstalled()
{
    bool res = false;
    const SC_HANDLE mngr = OpenSCManagerA(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (mngr) {
        const SC_HANDLE svc = OpenServiceA(mngr, serviceName, SERVICE_INTERROGATE);
        if (svc) {
            res = true;
            CloseServiceHandle(svc);
        }
        CloseServiceHandle(mngr);
    }
    return res;
}

bool StartupUtil::installService()
{
    bool res = false;
    const SC_HANDLE mngr = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (mngr) {
        const auto command = appFilePath() + " --service";
        const SC_HANDLE svc = CreateServiceA(mngr, serviceName, APP_NAME, SERVICE_ALL_ACCESS,
                SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
                command.toLatin1(), 0, 0, 0, 0, 0);
        if (svc) {
            res = true;
            CloseServiceHandle(svc);
        }
        CloseServiceHandle(mngr);
    }
    return res;
}

bool StartupUtil::uninstallService()
{
    bool res = false;
    const SC_HANDLE mngr = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (mngr) {
        const SC_HANDLE svc = OpenServiceA(mngr, serviceName, SERVICE_ALL_ACCESS | DELETE);
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

StartupUtil::StartupMode StartupUtil::getStartupMode()
{
    return isServiceInstalled()
            ? (isAutorunForAllUsers() ? StartupAllUsers : StartupAllUsersBackground)
            : (isAutorunForCurrentUser() ? StartupCurrentUser : StartupDisabled);
}

void StartupUtil::setStartupMode(StartupMode mode)
{
    removeAutorunForCurrentUser();
    removeAutorunForAllUsers();
    uninstallService();

    switch (mode) {
    case StartupDisabled:
        break;
    case StartupCurrentUser:
        setAutorunForCurrentUser();
        break;
    case StartupAllUsers:
        setAutorunForAllUsers();
        Q_FALLTHROUGH();
    case StartupAllUsersBackground:
        installService();
        break;
    }
}
