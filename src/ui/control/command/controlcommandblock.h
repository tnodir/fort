#ifndef CONTROLCOMMANDBLOCK_H
#define CONTROLCOMMANDBLOCK_H

#include "controlcommandbase.h"

class ControlCommandBlock : public ControlCommandBase
{
public:
    static bool processCommand(const ProcessCommandArgs &p);
};

#endif // CONTROLCOMMANDBLOCK_H
