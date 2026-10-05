#ifndef CONTROLCOMMANDBLOCK_H
#define CONTROLCOMMANDBLOCK_H

#include "controlcommandbase.h"

class ControlCommandBlock : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;
};

#endif // CONTROLCOMMANDBLOCK_H
