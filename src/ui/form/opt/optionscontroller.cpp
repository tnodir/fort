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

void OptionsController::setConfOthersEdited()
{
    if (!conf()->othersEdited()) {
        conf()->setOthersEdited(true);
        emit editedChanged(true);
    }
}

void OptionsController::setConfExtEdited()
{
    if (!conf()->extEdited()) {
        conf()->setExtEdited(true);
        emit editedChanged(true);
    }
}

void OptionsController::setConfIniEdited()
{
    if (!conf()->iniEdited()) {
        conf()->setIniEdited(true);
        emit editedChanged(true);
    }
}

void OptionsController::setConfFlagsEdited()
{
    if (!conf()->flagsEdited()) {
        conf()->setFlagsEdited(true);
        emit editedChanged(true);
    }
}

void OptionsController::setConfEdited()
{
    if (!conf()->edited()) {
        conf()->setEdited(true);
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
        setConfEdited();
    }
}

void OptionsController::closeWindow()
{
    fortManager()->closeOptionsWindow();
}

void OptionsController::save(bool closeOnSuccess)
{
    bool onlyFlags = true;
    if (conf()->flagsEdited() || conf()->edited()) {
        onlyFlags = !conf()->edited();
        if (!confManager()->saveConf(*conf(), onlyFlags))
            return;
    }

    if (conf()->othersEdited()) {
        emit saved();
    }

    confManager()->applySavedConf(conf(), onlyFlags);

    if (closeOnSuccess) {
        closeWindow();
    } else {
        confManager()->initConfToEdit();
        resetEdited();
    }
}
