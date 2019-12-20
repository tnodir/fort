#include "optionscontroller.h"

#include "../../fortmanager.h"
#include "../../fortsettings.h"

OptionsController::OptionsController(FortManager *fortManager,
                                     QObject *parent) :
    QObject(parent),
    m_confFlagsEdited(false),
    m_confEdited(false),
    m_othersEdited(false),
    m_fortManager(fortManager)
{
}

void OptionsController::setConfFlagsEdited(bool v)
{
    if (m_confFlagsEdited != v) {
        m_confFlagsEdited = v;
        emit editedChanged();
    }
}

void OptionsController::setConfEdited(bool v)
{
    if (m_confEdited != v) {
        m_confEdited = v;
        emit editedChanged();
    }
}

void OptionsController::setOthersEdited(bool v)
{
    if (m_othersEdited != v) {
        m_othersEdited = v;
        emit editedChanged();
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

FortSettings *OptionsController::settings()
{
    return m_fortManager->fortSettings();
}

FirewallConf *OptionsController::conf()
{
    return m_fortManager->firewallConfToEdit();
}

TaskManager *OptionsController::taskManager()
{
    return m_fortManager->taskManager();
}

void OptionsController::closeWindow()
{
    m_fortManager->closeOptionsWindow();
}

void OptionsController::save(bool closeOnSuccess)
{
    settings()->bulkUpdateBegin();

    emit aboutToSave();

    bool confSaved = true;
    if (confFlagsEdited() || confEdited()) {
        const bool confFlagsOnly = confFlagsEdited() && !confEdited();
        confSaved = closeOnSuccess
                ? m_fortManager->saveConf(confFlagsOnly)
                : m_fortManager->applyConf(confFlagsOnly);
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
    }
}
