#ifndef CONTROLCOMMANDBASE_H
#define CONTROLCOMMANDBASE_H

#include <control/control_types.h>

class ControlCommandBase
{
public:
    // Client: Read or write the files of the user
    virtual bool prepareClientArgs(QVariantList &args, ProcessCommandResult &r) const;
    virtual bool processClientResult(const QVariantList &args, ProcessCommandResult &r) const;

    virtual bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const = 0;

protected:
    static bool checkCommandActionPassword(
            ProcessCommandResult &r, quint32 action, quint32 passwordNotRequiredActions = 0);

    static void uncheckCommandActionPassword();
};

#endif // CONTROLCOMMANDBASE_H
