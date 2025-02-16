#ifndef CONTROLCOMMANDPROG_H
#define CONTROLCOMMANDPROG_H

#include "controlcommandbase.h"

class ControlCommandProg : public ControlCommandBase
{
public:
    static bool processCommand(const ProcessCommandArgs &p);
};

#endif // CONTROLCOMMANDPROG_H
