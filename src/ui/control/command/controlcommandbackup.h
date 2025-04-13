#ifndef CONTROLCOMMANDBACKUP_H
#define CONTROLCOMMANDBACKUP_H

#include "controlcommandbase.h"

class ControlCommandBackup : public ControlCommandBase
{
public:
    static bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);
};

#endif // CONTROLCOMMANDBACKUP_H
