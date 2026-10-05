#ifndef CONTROLCOMMANDZONE_H
#define CONTROLCOMMANDZONE_H

#include "controlcommandbase.h"

class ControlCommandZone : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;
};

#endif // CONTROLCOMMANDZONE_H
