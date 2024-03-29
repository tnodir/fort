#include "rule.h"

bool Rule::isNameEqual(const Rule &o) const
{
    return ruleName == o.ruleName;
}

bool Rule::isOptionsEqual(const Rule &o) const
{
    return enabled == o.enabled && blocked == o.blocked && exclusive == o.exclusive
            && acceptZones == o.acceptZones && rejectZones == o.rejectZones
            && ruleName == o.ruleName && notes == o.notes && ruleText == o.ruleText;
}
