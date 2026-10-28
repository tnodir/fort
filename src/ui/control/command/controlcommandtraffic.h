#ifndef CONTROLCOMMANDTRAFFIC_H
#define CONTROLCOMMANDTRAFFIC_H

#include "controlcommandbase.h"

class ControlCommandTraffic : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;
};

#endif // CONTROLCOMMANDTRAFFIC_H
