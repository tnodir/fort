#ifndef CONFSPEEDLIMITSWALKER_H
#define CONFSPEEDLIMITSWALKER_H

#include <QObject>

#include <functional>

class SpeedLimit;

struct WalkSpeedLimitsArgs
{
    quint8 maxLimitId = 0;
};

using walkSpeedLimitsCallback = bool(const SpeedLimit &limit);

class ConfSpeedLimitsWalker
{
public:
    virtual bool walkSpeedLimits(WalkSpeedLimitsArgs &wsla,
            const std::function<walkSpeedLimitsCallback> &func) const = 0;
};

#endif // CONFSPEEDLIMITSWALKER_H
