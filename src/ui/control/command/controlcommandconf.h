#ifndef CONTROLCOMMANDCONF_H
#define CONTROLCOMMANDCONF_H

#include "controlcommandbase.h"

class ControlCommandConf : public ControlCommandBase
{
public:
    static bool processCommand(const ProcessCommandArgs &p);
};

#endif // CONTROLCOMMANDCONF_H
