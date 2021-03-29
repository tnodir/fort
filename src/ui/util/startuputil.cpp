#include "startuputil.h"

#include <QCoreApplication>
#include <QSettings>

#include <fort_version.h>

#include "../service/servicemanager.h"

namespace {

const char *const regCurUserRun =
        R"(HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run)";
const char *const regAllUsersRun =
        R"(HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Run)";

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
    reg.setValue(APP_NAME, QString("\"%1\"").arg(QCoreApplication::applicationFilePath()));
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

StartupUtil::StartupMode StartupUtil::getStartupMode()
{
    return ServiceManager::isServiceInstalled()
            ? (isAutorunForAllUsers() ? StartupAllUsers : StartupAllUsersBackground)
            : (isAutorunForCurrentUser() ? StartupCurrentUser : StartupDisabled);
}

void StartupUtil::setStartupMode(StartupMode mode)
{
    removeAutorunForCurrentUser();
    removeAutorunForAllUsers();
    // TODO: ServiceManager::uninstallService();

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
        // TODO: ServiceManager::installService();
        break;
    }
}
