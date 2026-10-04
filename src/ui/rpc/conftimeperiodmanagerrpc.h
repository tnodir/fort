#ifndef CONFTIMEPERIODMANAGERRPC_H
#define CONFTIMEPERIODMANAGERRPC_H

#include <conf/conftimeperiodmanager.h>
#include <control/control_types.h>

class RpcManager;

class ConfTimePeriodManagerRpc : public ConfTimePeriodManager
{
    Q_OBJECT

public:
    explicit ConfTimePeriodManagerRpc(QObject *parent = nullptr);

    bool addOrUpdateTimePeriod(TimePeriod &period) override;
    bool deleteTimePeriod(quint8 periodId) override;
    bool updateTimePeriodName(quint8 periodId, const QString &name) override;
    bool updateTimePeriodEnabled(quint8 periodId, bool enabled) override;

    static QVariantList timePeriodToVarList(const TimePeriod &period);
    static TimePeriod varListToTimePeriod(const QVariantList &v);

    static bool processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);

    static void setupServerSignals(RpcManager *rpcManager);
};

#endif // CONFTIMEPERIODMANAGERRPC_H
