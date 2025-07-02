#include "controlcommandbase.h"

#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <util/ioc/ioccontainer.h>

bool ControlCommandBase::checkCommandActionPassword(
        ProcessCommandResult &r, quint32 action, quint32 passwordNotRequiredActions)
{
    if ((action & passwordNotRequiredActions) != 0)
        return true;

    if (!IoC<WindowManager>()->checkPassword()) {
        r.errorMessage = "Password required";
        return false;
    }

    IoC<FortSettings>()->setPasswordTemporaryChecked(true);

    return true;
}

void ControlCommandBase::uncheckCommandActionPassword()
{
    IoC<FortSettings>()->setPasswordTemporaryChecked(false);
}
