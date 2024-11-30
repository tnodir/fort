#include "rule.h"

bool Rule::isNameEqual(const Rule &o) const
{
    return ruleName == o.ruleName;
}

bool Rule::isOptionsEqual(const Rule &o) const
{
    return isFlagsEqual(o) && acceptZones == o.acceptZones && rejectZones == o.rejectZones
            && ruleName == o.ruleName && notes == o.notes && ruleText == o.ruleText;
}

bool Rule::isFlagsEqual(const Rule &o) const
{
    return enabled == o.enabled && blocked == o.blocked && exclusive == o.exclusive
            && terminate == o.terminate && terminateBlocked == o.terminateBlocked;
}

int Rule::terminateActionType() const
{
    return terminateBlocked ? TerminateBlock : TerminateAllow;
}

void Rule::setTerminateActionType(qint8 v)
{
    terminateBlocked = (v == TerminateBlock);
}
