#ifndef CONTROLCOMMANDRULE_H
#define CONTROLCOMMANDRULE_H

#include "controlcommandbase.h"

class ControlCommandRule : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;
};

#endif // CONTROLCOMMANDRULE_H
