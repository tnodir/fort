#ifndef CONTROLCOMMANDFILTER_H
#define CONTROLCOMMANDFILTER_H

#include "controlcommandbase.h"

class ControlCommandFilter : public ControlCommandBase
{
public:
    static bool processCommand(const ProcessCommandArgs &p);
};

#endif // CONTROLCOMMANDFILTER_H
