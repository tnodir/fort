#include "app.h"

bool App::isFlagsEqual(const App &o) const
{
    return isBaseFlagsEqual(o) && isExtraFlagsEqual(o);
}

bool App::isBaseFlagsEqual(const App &o) const
{
    return isWildcard == o.isWildcard && applyParent == o.applyParent && applyChild == o.applyChild
            && applySpecChild == o.applySpecChild && killChild == o.killChild
            && blockInbound == o.blockInbound && lanOnly == o.lanOnly;
}

bool App::isExtraFlagsEqual(const App &o) const
{
    return parked == o.parked && logStat == o.logStat && logBlockedConn == o.logBlockedConn
            && logAllowedConn == o.logAllowedConn && blocked == o.blocked
            && killProcess == o.killProcess;
}

bool App::isZonesEqual(const App &o) const
{
    return zones.accept_mask == o.zones.accept_mask && zones.reject_mask == o.zones.reject_mask;
}

bool App::isSpeedLimitsEqual(const App &o) const
{
    return inLimitEnabled == o.inLimitEnabled && outLimitEnabled == o.outLimitEnabled
            && speedLimits.in_limit_id == o.speedLimits.in_limit_id
            && speedLimits.out_limit_id == o.speedLimits.out_limit_id;
}

bool App::isPathsEqual(const App &o) const
{
    return appOriginPath == o.appOriginPath && appPath == o.appPath && iconPath == o.iconPath;
}

bool App::isScheduleEqual(const App &o) const
{
    return scheduleAction == o.scheduleAction && scheduleTime == o.scheduleTime;
}

bool App::isTextsEqual(const App &o) const
{
    return notes == o.notes && filtersText == o.filtersText;
}

bool App::isOptionsEqual(const App &o) const
{
    return isFlagsEqual(o) && isZonesEqual(o) && isSpeedLimitsEqual(o) && groups == o.groups
            && ruleId == o.ruleId && isTextsEqual(o) && isPathsEqual(o) && isScheduleEqual(o);
}

bool App::isNameEqual(const App &o) const
{
    return appName == o.appName;
}

bool App::isProcWild() const
{
    return applyParent || applyChild || killChild || killProcess;
}

bool App::hasGroup() const
{
    return groups != 0;
}

bool App::hasZone() const
{
    return zones.accept_mask != 0 || zones.reject_mask != 0;
}

bool App::hasSpeedLimit() const
{
    const FORT_SPEED_LIMIT_IDS limits = activeSpeedLimits();

    return limits.in_limit_id != 0 || limits.out_limit_id != 0;
}

FORT_SPEED_LIMIT_IDS App::activeSpeedLimits() const
{
    return {
        .in_limit_id = quint8(inLimitEnabled ? speedLimits.in_limit_id : 0),
        .out_limit_id = quint8(outLimitEnabled ? speedLimits.out_limit_id : 0),
    };
}
