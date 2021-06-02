#include "zonescontroller.h"

#include "../../conf/confmanager.h"
#include "../../conf/firewallconf.h"
#include "../../fortmanager.h"
#include "../../translationmanager.h"
#include "../../util/ioc/ioccontainer.h"

ZonesController::ZonesController(QObject *parent) : QObject(parent)
{
    connect(translationManager(), &TranslationManager::languageChanged, this,
            &ZonesController::retranslateUi);
}

FortManager *ZonesController::fortManager() const
{
    return IoC<FortManager>();
}

ConfManager *ZonesController::confManager() const
{
    return IoC<ConfManager>();
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
    return IoC<TranslationManager>();
}
