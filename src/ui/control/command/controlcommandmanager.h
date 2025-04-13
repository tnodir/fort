#ifndef CONTROLCOMMANDMANAGER_H
#define CONTROLCOMMANDMANAGER_H

#include <control/control_types.h>

class ControlCommandManager
{
public:
    static bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);
};

#endif // CONTROLCOMMANDMANAGER_H
