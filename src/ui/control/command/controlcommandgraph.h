#ifndef CONTROLCOMMANDGRAPH_H
#define CONTROLCOMMANDGRAPH_H

#include "controlcommandbase.h"

class ControlCommandGraph : public ControlCommandBase
{
public:
    static bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);
};

#endif // CONTROLCOMMANDGRAPH_H
