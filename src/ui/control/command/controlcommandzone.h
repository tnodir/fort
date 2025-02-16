#ifndef CONTROLCOMMANDZONE_H
#define CONTROLCOMMANDZONE_H

#include "controlcommandbase.h"

class ControlCommandZone : public ControlCommandBase
{
public:
    static bool processCommand(const ProcessCommandArgs &p);
};

#endif // CONTROLCOMMANDZONE_H
