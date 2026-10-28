#ifndef CONTROLCOMMANDPENDING_H
#define CONTROLCOMMANDPENDING_H

#include "controlcommandbase.h"

class ControlCommandPending : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;
};

#endif // CONTROLCOMMANDPENDING_H
