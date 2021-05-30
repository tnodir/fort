#include "statisticscontroller.h"

#include "../../conf/confmanager.h"
#include "../../conf/firewallconf.h"
#include "../../fortmanager.h"
#include "../../translationmanager.h"

StatisticsController::StatisticsController(FortManager *fortManager, QObject *parent) :
    QObject(parent), m_fortManager(fortManager)
{
    connect(translationManager(), &TranslationManager::languageChanged, this,
            &StatisticsController::retranslateUi);
}

FortSettings *StatisticsController::settings() const
{
    return fortManager()->settings();
}

ConfManager *StatisticsController::confManager() const
{
    return fortManager()->confManager();
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

StatManager *StatisticsController::statManager() const
{
    return fortManager()->statManager();
}

TranslationManager *StatisticsController::translationManager() const
{
    return TranslationManager::instance();
}
