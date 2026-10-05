#ifndef CONTROLCOMMANDRPC_H
#define CONTROLCOMMANDRPC_H

#include "controlcommandbase.h"

class ControlCommandRpc : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;
};

#endif // CONTROLCOMMANDRPC_H
