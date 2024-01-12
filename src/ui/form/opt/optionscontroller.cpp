#include "optionscontroller.h"

#include <QLoggingCategory>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/dialog/dialogutil.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <model/zonelistmodel.h>
#include <user/iniuser.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>

namespace {

const QLoggingCategory LC("optionsCtrl");

void showErrorMessage(const QString &errorMessage)
{
    IoC<WindowManager>()->showErrorBox(errorMessage, ConfManager::tr("Configuration Error"));
}

}

OptionsController::OptionsController(QObject *parent) : BaseController(parent)
{
    initConfManagerToEdit();
}

OptionsController::~OptionsController()
{
    confManager()->setConfToEdit(nullptr);
    confManager()->setIniUserToEdit(nullptr);
}

FirewallConf *OptionsController::confToEdit() const
{
    return confManager()->confToEdit();
}

IniUser *OptionsController::iniUserToEdit() const
{
    return confManager()->iniUserToEdit();
}

ZoneListModel *OptionsController::zoneListModel() const
{
    return IoC<ZoneListModel>();
}

bool OptionsController::anyEdited() const
{
    return m_iniUserEdited || confToEdit()->anyEdited();
}

void OptionsController::setOptEdited()
{
    if (!confToEdit()->optEdited()) {
        confToEdit()->setOptEdited();
        emitEdited(true);
    }
}

void OptionsController::setFlagsEdited()
{
    if (!confToEdit()->flagsEdited()) {
        confToEdit()->setFlagsEdited();
        emitEdited(true);
    }
}

void OptionsController::setIniEdited()
{
    if (!confToEdit()->iniEdited()) {
        confToEdit()->setIniEdited();
        emitEdited(true);
    }
}

void OptionsController::setTaskEdited()
{
    if (!confToEdit()->taskEdited()) {
        confToEdit()->setTaskEdited();
        emitEdited(true);
    }
}

void OptionsController::setIniUserEdited(bool flagsChanged)
{
    m_iniUserDataChanged |= !flagsChanged;
    m_iniUserFlagsChanged |= flagsChanged;

    if (!m_iniUserEdited) {
        m_iniUserEdited = true;
        emitEdited(true);
    }
}

void OptionsController::emitEdited(bool edited)
{
    emit editedChanged(edited);
}

void OptionsController::resetEdited()
{
    m_iniUserEdited = m_iniUserDataChanged = m_iniUserFlagsChanged = false;

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

    const bool isAnyEdited = this->anyEdited();
    const bool isConfEdited = confToEdit()->anyEdited();

    if (isAnyEdited) {
        qCDebug(LC) << "Conf save";
    }

    if (!confManager()->save(confToEdit())) {
        showErrorMessage(tr("Cannot save the options"));
        return;
    }

    if (m_iniUserEdited) {
        const bool onlyFlags = (m_iniUserFlagsChanged && !m_iniUserDataChanged && !isConfEdited);
        saveIniUser(onlyFlags);
    }

    if (closeOnSuccess) {
        closeWindow();
    } else if (isAnyEdited) {
        initConfManagerToEdit();
        resetEdited();
    }
}

void OptionsController::saveIniUser(bool onlyFlags)
{
    qCDebug(LC) << "IniUser save";

    iniUserToEdit()->saveAndClear();

    confManager()->saveIniUser(/*edited=*/true, onlyFlags);
}

void OptionsController::initConfManagerToEdit()
{
    confManager()->initConfToEdit();
    confManager()->initIniUserToEdit();
}

void OptionsController::exportBackup()
{
    const auto path = DialogUtil::getExistingDir(tr("Export Backup"));
    if (path.isEmpty())
        return;

    FileUtil::makePath(path);

    const QString outPath = FileUtil::pathSlash(path);

    if (confManager()->exportBackup(outPath)) {
        windowManager()->showInfoDialog(tr("Backup Exported Successfully"));
    } else {
        windowManager()->showErrorBox(tr("Cannot Export Backup"));
    }
}

void OptionsController::importBackup()
{
    const auto path = DialogUtil::getExistingDir(tr("Import Backup"));
    if (path.isEmpty())
        return;

    const QString inPath = FileUtil::pathSlash(path);

    const bool ok = confManager()->importBackup(inPath);

    windowManager()->processRestartRequired(
            ok ? tr("Backup Imported Successfully") : tr("Cannot Import Backup"));
}

void OptionsController::confirmImportBackup()
{
    windowManager()->showConfirmBox([&] { importBackup(); },
            tr("Program will be restarted after successful import. Continue?\n\n"
               "Make sure that you have a fresh backup."),
            tr("Import Backup"));
}

void OptionsController::closeWindow()
{
    windowManager()->closeOptionsWindow();
}
