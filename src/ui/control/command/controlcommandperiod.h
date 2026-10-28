#ifndef CONTROLCOMMANDPERIOD_H
#define CONTROLCOMMANDPERIOD_H

#include "controlcommandbase.h"

class ControlCommandPeriod : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;
};

#endif // CONTROLCOMMANDPERIOD_H
