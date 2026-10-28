#ifndef CONTROLCOMMANDUTIL_H
#define CONTROLCOMMANDUTIL_H

#include <control/control_types.h>

class ControlCommandUtil
{
public:
    // The password is checked temporarily: unchecked after the command by its manager
    static bool checkCommandActionPassword(
            ProcessCommandResult &r, quint32 action, quint32 passwordNotRequiredActions = 0);

    static void uncheckCommandActionPassword();
};

#endif // CONTROLCOMMANDUTIL_H
