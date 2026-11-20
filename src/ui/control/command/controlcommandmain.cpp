#include "controlcommandmain.h"

#include <fortglobal.h>
#include <manager/windowmanager.h>

using namespace Fort;

bool ControlCommandMain::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const
{
    const auto commandText = p.args.value(0).toString();

    if (commandText == "show") {
        return windowManager()->exposeMainWindow();
    }

    r.errorMessage = "Usage: home show";
    return false;
}
