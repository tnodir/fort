#ifndef CONTROLCOMMANDRPC_H
#define CONTROLCOMMANDRPC_H

#include "controlcommandbase.h"

class ControlCommandRpc : public ControlCommandBase
{
public:
    static bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);
};

#endif // CONTROLCOMMANDRPC_H
