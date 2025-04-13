#ifndef CONTROLCOMMANDPROG_H
#define CONTROLCOMMANDPROG_H

#include "controlcommandbase.h"

class ControlCommandProg : public ControlCommandBase
{
public:
    static bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);
};

#endif // CONTROLCOMMANDPROG_H
