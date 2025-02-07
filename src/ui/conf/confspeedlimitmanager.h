#ifndef CONFSPEEDLIMITMANAGER_H
#define CONFSPEEDLIMITMANAGER_H

#include <QObject>

#include <util/classhelpers.h>
#include <util/ioc/iocservice.h>

#include "confmanagerbase.h"

class SpeedLimit;

class ConfSpeedLimitManager : public ConfManagerBase, public IocService
{
    Q_OBJECT

public:
    explicit ConfSpeedLimitManager(QObject *parent = nullptr);
    CLASS_DELETE_COPY_MOVE(ConfSpeedLimitManager)

    virtual bool addOrUpdateSpeedLimit(SpeedLimit &limit);
    virtual bool deleteSpeedLimit(int limitId);
    virtual bool updateSpeedLimitName(int limitId, const QString &name);
    virtual bool updateSpeedLimitEnabled(int limitId, bool enabled);

signals:
    void speedLimitAdded();
    void speedLimitRemoved(int limitId);
    void speedLimitUpdated(int limitId);

private:
    bool updateDriverSpeedLimitFlag(int limitId, bool enabled);
};

#endif // CONFSPEEDLIMITMANAGER_H
