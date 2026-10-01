#ifndef SPEEDLIMITSCONTROLLER_H
#define SPEEDLIMITSCONTROLLER_H

#include <form/basecontroller.h>

class SpeedLimit;

class SpeedLimitsController : public BaseController
{
    Q_OBJECT

public:
    explicit SpeedLimitsController(QObject *parent = nullptr);

public slots:
    bool addOrUpdateSpeedLimit(SpeedLimit &limit);
    void deleteSpeedLimit(int limitId);
    bool updateSpeedLimitName(int limitId, const QString &name);
};

#endif // SPEEDLIMITSCONTROLLER_H
