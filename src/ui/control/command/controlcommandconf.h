#ifndef CONTROLCOMMANDCONF_H
#define CONTROLCOMMANDCONF_H

#include "controlcommandbase.h"

class ControlCommandConf : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;
};

#endif // CONTROLCOMMANDCONF_H
