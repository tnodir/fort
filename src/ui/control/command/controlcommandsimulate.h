#ifndef CONTROLCOMMANDSIMULATE_H
#define CONTROLCOMMANDSIMULATE_H

#include "controlcommandbase.h"

class ControlCommandSimulate : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;
};

#endif // CONTROLCOMMANDSIMULATE_H
