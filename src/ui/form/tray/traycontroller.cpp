#include "traycontroller.h"

#include "../../conf/confmanager.h"
#include "../../conf/firewallconf.h"
#include "../../fortmanager.h"
#include "../../translationmanager.h"

TrayController::TrayController(FortManager *fortManager, QObject *parent) :
    QObject(parent), m_fortManager(fortManager)
{
    connect(translationManager(), &TranslationManager::languageChanged, this,
            &TrayController::retranslateUi);
}

FortSettings *TrayController::settings() const
{
    return fortManager()->settings();
}

ConfManager *TrayController::confManager() const
{
    return fortManager()->confManager();
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
    return fortManager()->hotKeyManager();
}

TranslationManager *TrayController::translationManager() const
{
    return TranslationManager::instance();
}
