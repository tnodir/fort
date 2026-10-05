#ifndef CONTROLCOMMANDGROUP_H
#define CONTROLCOMMANDGROUP_H

#include "controlcommandbase.h"

class ControlCommandGroup : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;
};

#endif // CONTROLCOMMANDGROUP_H
