#include "programscontroller.h"

#include "../../conf/confmanager.h"
#include "../../conf/firewallconf.h"
#include "../../fortmanager.h"
#include "../../translationmanager.h"

ProgramsController::ProgramsController(FortManager *fortManager, QObject *parent) :
    QObject(parent), m_fortManager(fortManager)
{
    connect(translationManager(), &TranslationManager::languageChanged, this,
            &ProgramsController::retranslateUi);
}

FortSettings *ProgramsController::settings() const
{
    return fortManager()->settings();
}

ConfManager *ProgramsController::confManager() const
{
    return fortManager()->confManager();
}

FirewallConf *ProgramsController::conf() const
{
    return confManager()->conf();
}

IniOptions *ProgramsController::ini() const
{
    return &conf()->ini();
}

IniUser *ProgramsController::iniUser() const
{
    return confManager()->iniUser();
}

AppListModel *ProgramsController::appListModel() const
{
    return fortManager()->appListModel();
}

TranslationManager *ProgramsController::translationManager() const
{
    return TranslationManager::instance();
}
