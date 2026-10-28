#ifndef CONTROLCOMMANDRULE_H
#define CONTROLCOMMANDRULE_H

#include "controlcommandbase.h"

class ControlCommandRule : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;

protected:
    ClientFileArg clientFileArg(const QVariantList &args) const override;
};

#endif // CONTROLCOMMANDRULE_H
