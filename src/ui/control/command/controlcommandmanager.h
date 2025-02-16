#ifndef CONTROLCOMMANDMANAGER_H
#define CONTROLCOMMANDMANAGER_H

#include <control/control_types.h>

class ControlCommandManager
{
public:
    static bool processCommand(const ProcessCommandArgs &p);
};

#endif // CONTROLCOMMANDMANAGER_H
