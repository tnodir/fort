#include "controlcommandhome.h"

#include <fortglobal.h>
#include <manager/windowmanager.h>

using namespace Fort;

bool ControlCommandHome::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    const auto commandText = p.args.value(0).toString();

    if (commandText == "show") {
        return windowManager()->exposeHomeWindow();
    }

    r.errorMessage = "Usage: home show";
    return false;
}
