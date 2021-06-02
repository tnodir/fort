#include "programscontroller.h"

#include "../../conf/confmanager.h"
#include "../../conf/firewallconf.h"
#include "../../fortmanager.h"
#include "../../translationmanager.h"
#include "../../util/ioc/ioccontainer.h"

ProgramsController::ProgramsController(QObject *parent) : QObject(parent)
{
    connect(translationManager(), &TranslationManager::languageChanged, this,
            &ProgramsController::retranslateUi);
}

FortManager *ProgramsController::fortManager() const
{
    return IoC<FortManager>();
}

ConfManager *ProgramsController::confManager() const
{
    return IoC<ConfManager>();
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
    return IoC<TranslationManager>();
}
