#include "optionscontroller.h"

#include "../../conf/confmanager.h"
#include "../../fortmanager.h"
#include "../../fortsettings.h"
#include "../../translationmanager.h"

OptionsController::OptionsController(FortManager *fortManager, QObject *parent) :
    QObject(parent),
    m_confFlagsEdited(false),
    m_confEdited(false),
    m_othersEdited(false),
    m_fortManager(fortManager)
{
    connect(translationManager(), &TranslationManager::languageChanged, this,
            &OptionsController::retranslateUi);
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
    return fortManager()->confToEdit();
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

void OptionsController::setConfFlagsEdited(bool v)
{
    if (m_confFlagsEdited != v) {
        m_confFlagsEdited = v;
        emitEditedChanged();
    }
}

void OptionsController::setConfEdited(bool v)
{
    if (m_confEdited != v) {
        m_confEdited = v;
        emitEditedChanged();
    }
}

void OptionsController::setOthersEdited(bool v)
{
    if (m_othersEdited != v) {
        m_othersEdited = v;
        emitEditedChanged();
    }
}

void OptionsController::resetEdited()
{
    setConfFlagsEdited(false);
    setConfEdited(false);
    setOthersEdited(false);

    emit editResetted();
}

void OptionsController::initialize()
{
    // Settings/configuration was migrated?
    if (settings()->confMigrated()) {
        setConfEdited(true);
    }
}

void OptionsController::closeWindow()
{
    fortManager()->closeOptionsWindow();
}

void OptionsController::save(bool closeOnSuccess)
{
    settings()->bulkUpdateBegin();

    emit aboutToSave();

    bool confSaved = true;
    bool confFlagsOnly = true;
    if (confFlagsEdited() || confEdited()) {
        confFlagsOnly = confFlagsEdited() && !confEdited();
        confSaved = closeOnSuccess ? fortManager()->saveConf(confFlagsOnly)
                                   : fortManager()->applyConf(confFlagsOnly);
    }

    if (confSaved && othersEdited()) {
        emit saved();
    }

    settings()->bulkUpdateEnd();

    if (confSaved) {
        if (closeOnSuccess) {
            closeWindow();
        } else {
            resetEdited();
        }

        emit confManager()->confSaved(confFlagsOnly);
    }
}

void OptionsController::emitEditedChanged()
{
    emit editedChanged(anyEdited());
}
