#ifndef CONTROLCOMMANDZONE_H
#define CONTROLCOMMANDZONE_H

#include "controlcommandbase.h"

class ControlCommandZone : public ControlCommandBase
{
public:
    static bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);
};

#endif // CONTROLCOMMANDZONE_H
