#ifndef CONTROLCOMMANDFILTER_H
#define CONTROLCOMMANDFILTER_H

#include "controlcommandbase.h"

class ControlCommandFilter : public ControlCommandBase
{
public:
    static bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);
};

#endif // CONTROLCOMMANDFILTER_H
