#ifndef CONTROLCOMMANDBASE_H
#define CONTROLCOMMANDBASE_H

#include <control/control_types.h>

class ControlCommandBase
{
protected:
    static bool checkCommandActionPassword(
            ProcessCommandResult &r, quint32 action, quint32 passwordNotRequiredActions = 0);
};

#endif // CONTROLCOMMANDBASE_H
