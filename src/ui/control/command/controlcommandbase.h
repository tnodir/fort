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
};

#endif // CONTROLCOMMANDBASE_H
