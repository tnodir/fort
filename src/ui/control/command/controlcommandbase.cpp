#include "controlcommandbase.h"

#include <fortglobal.h>
#include <fortsettings.h>
#include <manager/windowmanager.h>

using namespace Fort;

bool ControlCommandBase::checkCommandActionPassword(
        ProcessCommandResult &r, quint32 action, quint32 passwordNotRequiredActions)
{
    if ((action & passwordNotRequiredActions) != 0)
        return true;

    if (r.disableCmdLine) {
        r.errorMessage = "Command line disabled";
        return false;
    }

    if (!windowManager()->checkPassword()) {
        r.errorMessage = "Password required";
        return false;
    }

    settings()->setPasswordTemporaryChecked(true);

    return true;
}

void ControlCommandBase::uncheckCommandActionPassword()
{
    settings()->setPasswordTemporaryChecked(false);
}
