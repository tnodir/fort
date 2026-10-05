#ifndef CONTROLCOMMANDMANAGER_H
#define CONTROLCOMMANDMANAGER_H

#include <control/control_types.h>

class ControlCommandManager
{
public:
    // Client: Prepare the arguments, post the command and process its result
    static bool processCommandClient(
            Control::Command command, QVariantList &args, ProcessCommandResult &r);

    static bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);
};

#endif // CONTROLCOMMANDMANAGER_H
