#ifndef CONTROLCOMMANDBACKUP_H
#define CONTROLCOMMANDBACKUP_H

#include "controlcommandbase.h"

class ControlCommandBackup : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;
};

#endif // CONTROLCOMMANDBACKUP_H
