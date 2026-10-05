#ifndef CONTROLCOMMANDFILTER_H
#define CONTROLCOMMANDFILTER_H

#include "controlcommandbase.h"

class ControlCommandFilter : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;
};

#endif // CONTROLCOMMANDFILTER_H
