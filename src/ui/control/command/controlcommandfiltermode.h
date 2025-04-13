#ifndef CONTROLCOMMANDFILTERMODE_H
#define CONTROLCOMMANDFILTERMODE_H

#include "controlcommandbase.h"

class ControlCommandFilterMode : public ControlCommandBase
{
public:
    static bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);
};

#endif // CONTROLCOMMANDFILTERMODE_H
