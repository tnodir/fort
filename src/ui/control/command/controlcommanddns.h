#ifndef CONTROLCOMMANDDNS_H
#define CONTROLCOMMANDDNS_H

#include "controlcommandbase.h"

class ControlCommandDns : public ControlCommandBase
{
public:
    bool processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const override;

    int resultWaitMsecs(const QVariantList &args) const override;
};

#endif // CONTROLCOMMANDDNS_H
