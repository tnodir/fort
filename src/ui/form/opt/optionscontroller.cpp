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

void OptionsController::setExtEdited()
{
    if (!conf()->extEdited()) {
        conf()->setExtEdited();
        emit editedChanged(true);
    }
}

void OptionsController::setIniEdited()
{
    if (!conf()->iniEdited()) {
        conf()->setIniEdited();
        emit editedChanged(true);
    }
}

void OptionsController::setFlagsEdited()
{
    if (!conf()->flagsEdited()) {
        conf()->setFlagsEdited();
        emit editedChanged(true);
    }
}

void OptionsController::setOptEdited()
{
    if (!conf()->optEdited()) {
        conf()->setOptEdited();
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

void OptionsController::closeWindow()
{
    fortManager()->closeOptionsWindow();
}
