#ifndef CONTROLCOMMANDHOME_H
#define CONTROLCOMMANDHOME_H

#include "controlcommandbase.h"

class ControlCommandHome : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;
};

#endif // CONTROLCOMMANDHOME_H
