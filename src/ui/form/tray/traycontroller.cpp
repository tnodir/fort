#include "traycontroller.h"

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <driver/drivermanager.h>
#include <fortmanager.h>
#include <fortsettings.h>
#include <manager/hotkeymanager.h>
#include <manager/translationmanager.h>
#include <manager/windowmanager.h>
#include <util/ioc/ioccontainer.h>

TrayController::TrayController(QObject *parent) : QObject(parent)
{
    connect(translationManager(), &TranslationManager::languageChanged, this,
            &TrayController::retranslateUi);
}

FortManager *TrayController::fortManager() const
{
    return IoC<FortManager>();
}

FortSettings *TrayController::settings() const
{
    return IoC<FortSettings>();
}

ConfManager *TrayController::confManager() const
{
    return IoC<ConfManager>();
}

FirewallConf *TrayController::conf() const
{
    return confManager()->conf();
}

IniOptions *TrayController::ini() const
{
    return &conf()->ini();
}

IniUser *TrayController::iniUser() const
{
    return confManager()->iniUser();
}

HotKeyManager *TrayController::hotKeyManager() const
{
    return IoC<HotKeyManager>();
}

DriverManager *TrayController::driverManager() const
{
    return IoC<DriverManager>();
}

TranslationManager *TrayController::translationManager() const
{
    return IoC<TranslationManager>();
}

WindowManager *TrayController::windowManager() const
{
    return IoC<WindowManager>();
}
