#include "homecontroller.h"

#include <conf/confmanager.h>
#include <fortglobal.h>
#include <fortsettings.h>

using namespace Fort;

HomeController::HomeController(QObject *parent) : BaseController(parent)
{
    connect(confManager(), &ConfManager::iniChanged, this, &HomeController::updatePasswordLocked);

    connect(settings(), &FortSettings::passwordCheckedChanged, this,
            &HomeController::updatePasswordLocked);

    updatePasswordLocked();
}

void HomeController::setPasswordLocked(bool v)
{
    if (m_passwordLocked == v)
        return;

    m_passwordLocked = v;
    emit passwordLockedChanged();
}

void HomeController::updatePasswordLocked()
{
    setPasswordLocked(settings()->isPasswordRequired());
}
