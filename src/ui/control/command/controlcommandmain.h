#ifndef CONTROLCOMMANDMAIN_H
#define CONTROLCOMMANDMAIN_H

#include "controlcommandbase.h"

class ControlCommandMain : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;

    bool needsWindows(const QVariantList & /*args*/) const override { return true; }
};

#endif // CONTROLCOMMANDMAIN_H
