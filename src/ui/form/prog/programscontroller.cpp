#include "programscontroller.h"

#include "../../fortmanager.h"
#include "../../log/logmanager.h"
#include "../../translationmanager.h"

ProgramsController::ProgramsController(FortManager *fortManager,
                                       QObject *parent) :
    QObject(parent),
    m_fortManager(fortManager)
{
    connect(translationManager(), &TranslationManager::languageChanged,
            this, &ProgramsController::retranslateUi);
}

FortSettings *ProgramsController::settings() const
{
    return fortManager()->settings();
}

FirewallConf *ProgramsController::conf() const
{
    return fortManager()->conf();
}

AppListModel *ProgramsController::appListModel() const
{
    return fortManager()->logManager()->appListModel();
}

TranslationManager *ProgramsController::translationManager() const
{
    return TranslationManager::instance();
}

void ProgramsController::closeWindow()
{
    fortManager()->closeProgramsWindow();
}
