#include "controlcommandhome.h"

#include <manager/windowmanager.h>
#include <util/ioc/ioccontainer.h>

bool ControlCommandHome::processCommand(const ProcessCommandArgs &p)
{
    const auto commandText = p.args.value(0).toString();

    if (commandText == "show") {
        return IoC<WindowManager>()->exposeHomeWindow();
    }

    p.errorMessage = "Usage: home show";
    return false;
}
