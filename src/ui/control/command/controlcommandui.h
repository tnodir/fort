#ifndef CONTROLCOMMANDUI_H
#define CONTROLCOMMANDUI_H

#include "controlcommandbase.h"

class ControlCommandUi : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;

    bool needsWindows(const QVariantList & /*args*/) const override { return true; }

    int resultWaitMsecs(const QVariantList &args) const override;

protected:
    ClientFileArg clientFileArg(const QVariantList &args) const override;
};

#endif // CONTROLCOMMANDUI_H
