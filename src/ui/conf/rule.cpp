#include "rule.h"

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
    return enabled == o.enabled && trayMenu == o.trayMenu && blocked == o.blocked
            && exclusive == o.exclusive && inlineZones == o.inlineZones;
}

bool Rule::isTerminateFlagsEqual(const Rule &o) const
{
    return terminate == o.terminate && terminateBlocked == o.terminateBlocked
            && terminateDrop == o.terminateDrop && terminateAlert == o.terminateAlert;
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
