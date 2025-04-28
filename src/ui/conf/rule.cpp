#include "rule.h"

bool Rule::isNameEqual(const Rule &o) const
{
    return ruleName == o.ruleName;
}

bool Rule::isOptionsEqual(const Rule &o) const
{
    return isFlagsEqual(o) && isZonesEqual(o) && notes == o.notes && ruleText == o.ruleText;
}

bool Rule::isFlagsEqual(const Rule &o) const
{
    return enabled == o.enabled && blocked == o.blocked && exclusive == o.exclusive
            && terminate == o.terminate && terminateBlocked == o.terminateBlocked;
}

bool Rule::isZonesEqual(const Rule &o) const
{
    return zones.accept_mask == o.zones.accept_mask && zones.reject_mask == o.zones.reject_mask;
}

int Rule::terminateActionType() const
{
    return terminateBlocked ? TerminateBlock : TerminateAllow;
}

void Rule::setTerminateActionType(qint8 v)
{
    terminateBlocked = (v == TerminateBlock);
}
