#ifndef CONTROLCOMMANDPROG_H
#define CONTROLCOMMANDPROG_H

#include "controlcommandbase.h"

class ControlCommandProg : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;

    bool needsWindows(const QVariantList &args) const override;

protected:
    ClientFileArg clientFileArg(const QVariantList &args) const override;
};

#endif // CONTROLCOMMANDPROG_H
