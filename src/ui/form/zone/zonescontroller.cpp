#include "zonescontroller.h"

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <manager/translationmanager.h>
#include <manager/windowmanager.h>
#include <model/zonelistmodel.h>
#include <task/taskmanager.h>
#include <util/ioc/ioccontainer.h>

ZonesController::ZonesController(QObject *parent) : QObject(parent)
{
    connect(translationManager(), &TranslationManager::languageChanged, this,
            &ZonesController::retranslateUi);
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

TranslationManager *ZonesController::translationManager() const
{
    return IoC<TranslationManager>();
}

WindowManager *ZonesController::windowManager() const
{
    return IoC<WindowManager>();
}

TaskManager *ZonesController::taskManager() const
{
    return IoC<TaskManager>();
}

ZoneListModel *ZonesController::zoneListModel() const
{
    return IoC<ZoneListModel>();
}
