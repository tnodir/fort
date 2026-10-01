#ifndef CONFSPEEDLIMITSWALKER_H
#define CONFSPEEDLIMITSWALKER_H

#include <QObject>

#include <functional>

class SpeedLimit;

using walkSpeedLimitsCallback = bool(const SpeedLimit &limit);

class ConfSpeedLimitsWalker
{
public:
    virtual bool walkSpeedLimits(const std::function<walkSpeedLimitsCallback> &func) const = 0;
};

#endif // CONFSPEEDLIMITSWALKER_H
