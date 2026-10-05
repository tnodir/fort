#ifndef CONTROLCOMMANDFILTERMODE_H
#define CONTROLCOMMANDFILTERMODE_H

#include "controlcommandbase.h"

class ControlCommandFilterMode : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;
};

#endif // CONTROLCOMMANDFILTERMODE_H
