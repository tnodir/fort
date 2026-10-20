#include "rule.h"

#include <common/fortconf.h>

// The name, options, type and rule set
bool Rule::isContentEqual(const Rule &o) const
{
    return isNameEqual(o) && isOptionsEqual(o) && ruleType == o.ruleType && ruleSet == o.ruleSet;
}

bool Rule::isNameEqual(const Rule &o) const
{
    return ruleName == o.ruleName;
}

bool Rule::isOptionsEqual(const Rule &o) const
{
    return isFlagsEqual(o) && isTerminateFlagsEqual(o) && isLogFlagsEqual(o) && isZonesEqual(o)
            && isPeriodEqual(o) && notes == o.notes && ruleText == o.ruleText;
}

bool Rule::isFlagsEqual(const Rule &o) const
{
    return enabled == o.enabled && trayMenu == o.trayMenu && blocked == o.blocked && ask == o.ask
            && exclusive == o.exclusive && inlineZones == o.inlineZones;
}

bool Rule::isTerminateFlagsEqual(const Rule &o) const
{
    return terminate == o.terminate && terminateBlocked == o.terminateBlocked
            && terminateDrop == o.terminateDrop && terminateAsk == o.terminateAsk
            && terminateAlert == o.terminateAlert;
}

bool Rule::isLogFlagsEqual(const Rule &o) const
{
    return logAllowedConn == o.logAllowedConn && logBlockedConn == o.logBlockedConn;
}

bool Rule::isZonesEqual(const Rule &o) const
{
    return zones.accept_mask == o.zones.accept_mask && zones.reject_mask == o.zones.reject_mask;
}

bool Rule::isPeriodEqual(const Rule &o) const
{
    return periodEnabled == o.periodEnabled && periodId == o.periodId;
}

quint8 Rule::terminateActionTypeId() const
{
    if (!terminateBlocked)
        return FORT_RULE_FILTER_ACTION_ALLOW;

    if (terminateAsk)
        return FORT_RULE_FILTER_ACTION_ASK;

    return terminateDrop ? FORT_RULE_FILTER_ACTION_DROP : FORT_RULE_FILTER_ACTION_BLOCK;
}

void Rule::setTerminateActionTypeId(quint8 actionTypeId)
{
    terminateBlocked = (actionTypeId != FORT_RULE_FILTER_ACTION_ALLOW);
    terminateDrop = (actionTypeId == FORT_RULE_FILTER_ACTION_DROP);
    terminateAsk = (actionTypeId == FORT_RULE_FILTER_ACTION_ASK);
}
