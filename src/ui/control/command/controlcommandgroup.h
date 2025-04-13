#ifndef CONTROLCOMMANDGROUP_H
#define CONTROLCOMMANDGROUP_H

#include "controlcommandbase.h"

class ControlCommandGroup : public ControlCommandBase
{
public:
    static bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);
};

#endif // CONTROLCOMMANDGROUP_H
