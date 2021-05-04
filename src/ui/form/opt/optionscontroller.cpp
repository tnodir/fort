#include "optionscontroller.h"

#include "../../conf/confmanager.h"
#include "../../conf/firewallconf.h"
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

    bool confSaved = true;
    bool onlyFlags = true;
    if (confFlagsEdited() || confEdited()) {
        onlyFlags = confFlagsEdited() && !confEdited();
        confSaved = confManager()->saveToDbIni(*conf(), onlyFlags);
    }

    if (confSaved) {
        if (othersEdited()) {
            emit saved();
        }

        confManager()->applySavedConf(conf(), onlyFlags);

        if (closeOnSuccess) {
            closeWindow();
        } else {
            if (!conf()) {
                confManager()->initConfToEdit();
            }
            resetEdited();
        }
    }

    settings()->bulkUpdateEnd();
}

void OptionsController::applyImmediateFlags()
{
    confManager()->save(conf(), true, true);
}

void OptionsController::emitEditedChanged()
{
    emit editedChanged(anyEdited());
}
