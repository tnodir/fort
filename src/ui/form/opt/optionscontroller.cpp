#include "optionscontroller.h"

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <model/zonelistmodel.h>
#include <user/iniuser.h>
#include <util/ioc/ioccontainer.h>

OptionsController::OptionsController(QObject *parent) :
    BaseController(parent),
    m_iniUserEdited(false),
    m_iniUserDataChanged(false),
    m_iniUserFlagsChanged(false)
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

    if (!confManager()->save(confToEdit()))
        return;

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
    iniUserToEdit()->save();
    iniUserToEdit()->clear();

    confManager()->saveIniUser(/*edited=*/true, onlyFlags);
}

void OptionsController::initConfManagerToEdit()
{
    confManager()->initConfToEdit();
    confManager()->initIniUserToEdit();
}

void OptionsController::closeWindow()
{
    windowManager()->closeOptionsWindow();
}
