#include "optionscontroller.h"

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <model/zonelistmodel.h>
#include <user/iniuser.h>
#include <util/ioc/ioccontainer.h>

OptionsController::OptionsController(QObject *parent) :
    BaseController(parent), m_iniUserEdited(false), m_iniUserFlagsChanged(false)
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
    return m_iniUserEdited || conf()->anyEdited();
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

void OptionsController::setIniUserEdited(bool flagsChanged)
{
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
    m_iniUserEdited = m_iniUserFlagsChanged = false;

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

    if (!confManager()->save(conf()))
        return;

    if (m_iniUserEdited) {
        saveIniUser();
    }

    if (closeOnSuccess) {
        closeWindow();
    } else if (isAnyEdited) {
        initConfManagerToEdit();
        resetEdited();
    }
}

void OptionsController::saveIniUser()
{
    iniUser()->save();
    iniUser()->clear();

    confManager()->saveIniUser(m_iniUserFlagsChanged);
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
