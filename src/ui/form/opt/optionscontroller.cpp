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

IniUser *OptionsController::iniUser() const
{
    return confManager()->iniUser();
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

void OptionsController::setOptEdited()
{
    if (!conf()->optEdited()) {
        conf()->setOptEdited();
        emitEdited(true);
    }
}

void OptionsController::setFlagsEdited()
{
    if (!conf()->flagsEdited()) {
        conf()->setFlagsEdited();
        emitEdited(true);
    }
}

void OptionsController::setIniEdited()
{
    if (!conf()->iniEdited()) {
        conf()->setIniEdited();
        emitEdited(true);
    }
}

void OptionsController::setTaskEdited()
{
    if (!conf()->taskEdited()) {
        conf()->setTaskEdited();
        emitEdited(true);
    }
}

void OptionsController::emitEdited(bool edited)
{
    emit editedChanged(edited);
}

void OptionsController::resetEdited()
{
    emitEdited(false);
    emit editResetted();
}

void OptionsController::initialize()
{
    // Settings/configuration was migrated?
    if (settings()->wasMigrated()) {
        setOptEdited();
    }
}

void OptionsController::save(bool closeOnSuccess)
{
    emit aboutToSave();

    const bool isAnyEdited = conf()->anyEdited();
    if (!isAnyEdited) {
        emitEdited(false);
    } else if (!confManager()->save(conf()))
        return;

    if (closeOnSuccess) {
        closeWindow();
    } else if (isAnyEdited) {
        confManager()->initConfToEdit();
        resetEdited();
    }
}

void OptionsController::closeWindow()
{
    fortManager()->closeOptionsWindow();
}
