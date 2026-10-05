#include "controlcommandgraph.h"

#include <fortglobal.h>
#include <manager/windowmanager.h>

using namespace Fort;

bool ControlCommandGraph::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const
{
    const auto commandText = p.args.value(0).toString();

    if (commandText == "show") {
        return windowManager()->setGraphWindowVisible(true);
    }

    if (commandText == "hide") {
        return windowManager()->setGraphWindowVisible(false);
    }

    if (commandText == "switch") {
        return windowManager()->setGraphWindowVisible();
    }

    r.errorMessage = "Usage: graph show|hide|switch";
    return false;
}
