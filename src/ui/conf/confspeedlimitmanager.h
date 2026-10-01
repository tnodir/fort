#ifndef CONFSPEEDLIMITMANAGER_H
#define CONFSPEEDLIMITMANAGER_H

#include <QHash>
#include <QObject>

#include <util/classhelpers.h>
#include <util/conf/confspeedlimitswalker.h>
#include <util/ioc/iocservice.h>

#include "confmanagerbase.h"

class SpeedLimit;

class ConfSpeedLimitManager : public ConfManagerBase,
                              public ConfSpeedLimitsWalker,
                              public IocService
{
    Q_OBJECT

public:
    explicit ConfSpeedLimitManager(QObject *parent = nullptr);
    CLASS_DELETE_COPY_MOVE(ConfSpeedLimitManager)

    void setUp() override;

    QString speedLimitNameById(quint8 limitId);

    virtual bool addOrUpdateSpeedLimit(SpeedLimit &limit);
    virtual bool deleteSpeedLimit(quint8 limitId);
    virtual bool updateSpeedLimitName(quint8 limitId, const QString &name);
    virtual bool updateSpeedLimitEnabled(quint8 limitId, bool enabled);

    bool walkSpeedLimits(const std::function<walkSpeedLimitsCallback> &func) const override;

    void updateDriverSpeedLimits();
    void updateDriverSpeedLimitFlags();

signals:
    void speedLimitAdded();
    void speedLimitRemoved(quint8 limitId);
    void speedLimitUpdated();

private:
    void setupSpeedLimitNamesCache();
    void clearSpeedLimitNamesCache();

    static void fillSpeedLimit(SpeedLimit &limit, const SqliteStmt &stmt);

private:
    mutable QHash<quint8, QString> m_speedLimitNamesCache;
};

#endif // CONFSPEEDLIMITMANAGER_H
