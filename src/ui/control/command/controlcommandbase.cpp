#include "controlcommandbase.h"

#include <manager/windowmanager.h>
#include <util/ioc/ioccontainer.h>

bool ControlCommandBase::checkCommandActionPassword(
        ProcessCommandResult &r, quint32 action, quint32 passwordNotRequiredActions)
{
    if ((action & passwordNotRequiredActions) != 0)
        return true;

    if (!IoC<WindowManager>()->checkPassword(/*temporary=*/true)) {
        r.errorMessage = "Password required";
        return false;
    }

    return true;
}
