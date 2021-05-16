#include "zonescontroller.h"

#include "../../conf/confmanager.h"
#include "../../conf/firewallconf.h"
#include "../../fortmanager.h"
#include "../../translationmanager.h"

ZonesController::ZonesController(FortManager *fortManager, QObject *parent) :
    QObject(parent), m_fortManager(fortManager)
{
    connect(translationManager(), &TranslationManager::languageChanged, this,
            &ZonesController::retranslateUi);
}

FortSettings *ZonesController::settings() const
{
    return fortManager()->settings();
}

ConfManager *ZonesController::confManager() const
{
    return fortManager()->confManager();
}

FirewallConf *ZonesController::conf() const
{
    return confManager()->conf();
}

IniOptions *ZonesController::ini() const
{
    return &conf()->ini();
}

IniUser *ZonesController::iniUser() const
{
    return confManager()->iniUser();
}

ZoneListModel *ZonesController::zoneListModel() const
{
    return fortManager()->zoneListModel();
}

TranslationManager *ZonesController::translationManager() const
{
    return TranslationManager::instance();
}
