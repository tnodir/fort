#include "traycontroller.h"

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

FirewallConf *TrayController::conf() const
{
    return fortManager()->conf();
}

HotKeyManager *TrayController::hotKeyManager() const
{
    return fortManager()->hotKeyManager();
}

TranslationManager *TrayController::translationManager() const
{
    return TranslationManager::instance();
}
