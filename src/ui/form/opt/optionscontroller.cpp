#include "optionscontroller.h"

#include <QLoggingCategory>

#include <conf/confmanager.h>
#include <fortglobal.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <user/usersettings.h>

using namespace Fort;

namespace {

const QLoggingCategory LC("optionsCtrl");

void showErrorMessage(const QString &errorMessage)
{
    windowManager()->showErrorBox(errorMessage, ConfManager::tr("Configuration Error"));
}

void showErrorMessageDetails(const QString &errorMessage, const QString &details)
{
    QString text = errorMessage;

    if (!details.isEmpty()) {
        text += ":\n" + details;
    }

    showErrorMessage(text);
}

}

OptionsController::OptionsController(QObject *parent) :
    BaseController(parent), m_iniOptToEdit(settings()), m_iniUserToEdit(userSettings())
{
    initConfToEdit();
}

bool OptionsController::anyEdited() const
{
    return m_iniUserEdited || confToEdit().anyEdited();
}

void OptionsController::setOptEdited()
{
    if (!confToEdit().optEdited()) {
        confToEdit().setOptEdited();
        emitEdited(true);
    }
}

void OptionsController::setFlagsEdited(FirewallConf::FlagGroups groups)
{
    const bool wasEdited = confToEdit().flagsEdited();

    confToEdit().setFlagsEdited(groups);

    if (!wasEdited) {
        emitEdited(true);
    }
}

void OptionsController::setIniEdited()
{
    if (!confToEdit().iniEdited()) {
        confToEdit().setIniEdited();
        emitEdited(true);
    }
}

void OptionsController::setTaskEdited()
{
    if (!confToEdit().taskEdited()) {
        confToEdit().setTaskEdited();
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

void OptionsController::save(bool closeOnSuccess)
{
    emit aboutToSave();

    const bool isAnyEdited = this->anyEdited();
    const bool isConfEdited = confToEdit().anyEdited();

    if (isAnyEdited) {
        qCDebug(LC) << "Conf save";
    }

    if (!confManager()->save(confToEdit(), iniOptToEdit())) {
        showErrorMessageDetails(tr("Cannot save the options"), confManager()->errorMessage());
        return;
    }

    if (m_iniUserEdited) {
        const bool onlyFlags = (m_iniUserFlagsChanged && !m_iniUserDataChanged && !isConfEdited);
        saveIniUser(onlyFlags);
    }

    if (closeOnSuccess) {
        closeWindow();
    } else if (isAnyEdited) {
        resetConfToEdit();
        resetEdited();
    }
}

void OptionsController::saveIniUser(bool onlyFlags)
{
    qCDebug(LC) << "IniUser save";

    confManager()->saveIniUser(iniUserToEdit(), onlyFlags);
}

void OptionsController::initConfToEdit()
{
    confToEdit().copy(conf());
}

void OptionsController::resetConfToEdit()
{
    confToEdit().resetEdited();

    iniOptToEdit().clear();
    iniUserToEdit().clear();
}

void OptionsController::closeWindow()
{
    windowManager()->closeOptionsWindow();
}
