#ifndef CONFSPEEDLIMITMANAGERRPC_H
#define CONFSPEEDLIMITMANAGERRPC_H

#include <conf/confspeedlimitmanager.h>
#include <control/control_types.h>

class RpcManager;

class ConfSpeedLimitManagerRpc : public ConfSpeedLimitManager
{
    Q_OBJECT

public:
    explicit ConfSpeedLimitManagerRpc(QObject *parent = nullptr);

    bool addOrUpdateSpeedLimit(SpeedLimit &limit) override;
    bool deleteSpeedLimit(quint8 limitId) override;
    bool updateSpeedLimitName(quint8 limitId, const QString &name) override;
    bool updateSpeedLimitEnabled(quint8 limitId, bool enabled) override;

    static QVariantList speedLimitToVarList(const SpeedLimit &limit);
    static SpeedLimit varListToSpeedLimit(const QVariantList &v);

    static bool processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);

    static void setupServerSignals(RpcManager *rpcManager);
};

#endif // CONFSPEEDLIMITMANAGERRPC_H
