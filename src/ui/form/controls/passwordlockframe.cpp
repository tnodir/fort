#include "passwordlockframe.h"

#include <conf/confmanager.h>
#include <fortglobal.h>
#include <fortsettings.h>

using namespace Fort;

PasswordLockFrame::PasswordLockFrame(QWidget *parent) : QFrame(parent)
{
    connect(confManager(), &ConfManager::iniChanged, this, &PasswordLockFrame::updateLocked);

    connect(settings(), &FortSettings::passwordCheckedChanged, this,
            &PasswordLockFrame::updateLocked);

    updateLocked();
}

void PasswordLockFrame::updateLocked()
{
    setEnabled(!settings()->isPasswordRequired());
}
