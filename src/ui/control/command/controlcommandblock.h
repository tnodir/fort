#ifndef CONTROLCOMMANDBLOCK_H
#define CONTROLCOMMANDBLOCK_H

#include "controlcommandbase.h"

class ControlCommandBlock : public ControlCommandBase
{
public:
    static bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);
};

#endif // CONTROLCOMMANDBLOCK_H
