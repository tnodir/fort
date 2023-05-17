#include "homecontroller.h"

#include <conf/confmanager.h>
#include <fortsettings.h>
#include <util/ioc/ioccontainer.h>

HomeController::HomeController(QObject *parent) : BaseController(parent)
{
    connect(IoC<ConfManager>(), &ConfManager::iniChanged, this,
            &HomeController::updatePasswordLocked);
    connect(IoC<FortSettings>(), &FortSettings::passwordCheckedChanged, this,
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
    auto settings = IoC<FortSettings>();

    setPasswordLocked(settings->isPasswordRequired());
}
