#ifndef CONTROLCOMMANDADDR_H
#define CONTROLCOMMANDADDR_H

#include "controlcommandbase.h"

class ControlCommandAddr : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;

protected:
    ClientFileArg clientFileArg(const QVariantList &args) const override;
};

#endif // CONTROLCOMMANDADDR_H
