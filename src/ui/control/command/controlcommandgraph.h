#ifndef CONTROLCOMMANDGRAPH_H
#define CONTROLCOMMANDGRAPH_H

#include "controlcommandbase.h"

class ControlCommandGraph : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;

    bool needsWindows(const QVariantList & /*args*/) const override { return true; }
};

#endif // CONTROLCOMMANDGRAPH_H
