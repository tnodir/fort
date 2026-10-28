#ifndef CONTROLCOMMANDWINDOW_H
#define CONTROLCOMMANDWINDOW_H

#include "controlcommandbase.h"

class ControlCommandWindow : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;
};

#endif // CONTROLCOMMANDWINDOW_H
