#ifndef CONTROLCOMMANDBASE_H
#define CONTROLCOMMANDBASE_H

#include <control/control_types.h>

class ControlCommandBase
{
public:
    virtual bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const = 0;

protected:
    static bool checkCommandActionPassword(
            ProcessCommandResult &r, quint32 action, quint32 passwordNotRequiredActions = 0);

    static void uncheckCommandActionPassword();
};

#endif // CONTROLCOMMANDBASE_H
