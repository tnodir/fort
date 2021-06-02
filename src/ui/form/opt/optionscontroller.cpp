#include "optionscontroller.h"

#include "../../conf/confmanager.h"
#include "../../conf/firewallconf.h"
#include "../../driver/drivermanager.h"
#include "../../fortmanager.h"
#include "../../fortsettings.h"
#include "../../model/zonelistmodel.h"
#include "../../task/taskmanager.h"
#include "../../translationmanager.h"
#include "../../util/ioc/ioccontainer.h"

OptionsController::OptionsController(QObject *parent) : QObject(parent)
{
    confManager()->initConfToEdit();

    connect(translationManager(), &TranslationManager::languageChanged, this,
            &OptionsController::retranslateUi);
}

OptionsController::~OptionsController()
{
    confManager()->setConfToEdit(nullptr);
}

FortManager *OptionsController::fortManager() const
{
    return IoC<FortManager>();
}

FortSettings *OptionsController::settings() const
{
    return IoC<FortSettings>();
}

ConfManager *OptionsController::confManager() const
{
    return IoC<ConfManager>();
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
    return IoC<TaskManager>();
}

DriverManager *OptionsController::driverManager() const
{
    return IoC<DriverManager>();
}

TranslationManager *OptionsController::translationManager() const
{
    return IoC<TranslationManager>();
}

ZoneListModel *OptionsController::zoneListModel() const
{
    return IoC<ZoneListModel>();
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
