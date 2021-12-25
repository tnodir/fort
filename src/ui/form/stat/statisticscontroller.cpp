#include "statisticscontroller.h"

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <fortmanager.h>
#include <manager/translationmanager.h>
#include <manager/windowmanager.h>
#include <util/ioc/ioccontainer.h>

StatisticsController::StatisticsController(QObject *parent) : QObject(parent)
{
    connect(translationManager(), &TranslationManager::languageChanged, this,
            &StatisticsController::retranslateUi);
}

FortManager *StatisticsController::fortManager() const
{
    return IoC<FortManager>();
}

ConfManager *StatisticsController::confManager() const
{
    return IoC<ConfManager>();
}

FirewallConf *StatisticsController::conf() const
{
    return confManager()->conf();
}

IniOptions *StatisticsController::ini() const
{
    return &conf()->ini();
}

IniUser *StatisticsController::iniUser() const
{
    return confManager()->iniUser();
}

TranslationManager *StatisticsController::translationManager() const
{
    return IoC<TranslationManager>();
}

WindowManager *StatisticsController::windowManager() const
{
    return IoC<WindowManager>();
}
