#ifndef CONTROLCOMMANDRULE_H
#define CONTROLCOMMANDRULE_H

#include "controlcommandbase.h"

class ControlCommandRule : public ControlCommandBase
{
public:
    bool prepareClientArgs(QVariantList &args, ProcessCommandResult &r) const override;

    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;

protected:
    ClientFileArg clientFileArg(const QVariantList &args) const override;
};

#endif // CONTROLCOMMANDRULE_H
