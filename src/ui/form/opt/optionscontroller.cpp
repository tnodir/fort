#include "optionscontroller.h"

#include "../../conf/confmanager.h"
#include "../../conf/firewallconf.h"
#include "../../fortmanager.h"
#include "../../fortsettings.h"
#include "../../translationmanager.h"

OptionsController::OptionsController(FortManager *fortManager, QObject *parent) :
    QObject(parent), m_fortManager(fortManager)
{
    confManager()->initConfToEdit();

    connect(translationManager(), &TranslationManager::languageChanged, this,
            &OptionsController::retranslateUi);
}

OptionsController::~OptionsController()
{
    confManager()->setConfToEdit(nullptr);
}

FortSettings *OptionsController::settings() const
{
    return fortManager()->settings();
}

ConfManager *OptionsController::confManager() const
{
    return fortManager()->confManager();
}

FirewallConf *OptionsController::conf() const
{
    return confManager()->confToEdit();
}

IniOptions *OptionsController::ini() const
{
    return &conf()->ini();
}

TaskManager *OptionsController::taskManager() const
{
    return fortManager()->taskManager();
}

DriverManager *OptionsController::driverManager() const
{
    return fortManager()->driverManager();
}

TranslationManager *OptionsController::translationManager() const
{
    return TranslationManager::instance();
}

ZoneListModel *OptionsController::zoneListModel() const
{
    return fortManager()->zoneListModel();
}

void OptionsController::setOthersEdited()
{
    if (!conf()->othersEdited()) {
        conf()->setOthersEdited(true);
        emit editedChanged(true);
    }
}

void OptionsController::setExtEdited()
{
    if (!conf()->extEdited()) {
        conf()->setExtEdited(true);
        emit editedChanged(true);
    }
}

void OptionsController::setGraphEdited()
{
    if (!conf()->graphEdited()) {
        conf()->setGraphEdited(true);
        emit editedChanged(true);
    }
}

void OptionsController::setIniEdited()
{
    if (!conf()->iniEdited()) {
        conf()->setIniEdited(true);
        emit editedChanged(true);
    }
}

void OptionsController::setFlagsEdited()
{
    if (!conf()->flagsEdited()) {
        conf()->setFlagsEdited(true);
        emit editedChanged(true);
    }
}

void OptionsController::setOptEdited()
{
    if (!conf()->optEdited()) {
        conf()->setOptEdited(true);
        emit editedChanged(true);
    }
}

void OptionsController::resetEdited()
{
    emit editedChanged(false);
    emit editResetted();
}

void OptionsController::initialize()
{
    // Settings/configuration was migrated?
    if (settings()->confMigrated()) {
        setOptEdited();
    }
}

void OptionsController::closeWindow()
{
    fortManager()->closeOptionsWindow();
}

void OptionsController::save(bool closeOnSuccess)
{
    emit aboutToSave();

    if (!confManager()->saveConf(*conf()))
        return;

    confManager()->applySavedConf(conf());

    if (closeOnSuccess) {
        closeWindow();
    } else {
        confManager()->initConfToEdit();
        resetEdited();
    }
}
