#include "conftimeperiodmanagerrpc.h"

#include <fortglobal.h>
#include <rpc/rpcmanager.h>

using namespace Fort;

namespace {

QVariantList intervalsToVarList(const TimePeriodIntervals &intervals)
{
    QVariantList list;

    for (const TimePeriodInterval &interval : intervals) {
        list << QVariant(QVariantList { interval.weekDays, interval.timeFrom, interval.timeTo });
    }

    return list;
}

TimePeriodIntervals varListToIntervals(const QVariantList &list)
{
    TimePeriodIntervals intervals;

    for (const QVariant &item : list) {
        const QVariantList v = item.toList();

        TimePeriodInterval interval;
        interval.weekDays = v.value(0).toUInt();
        interval.timeFrom = v.value(1).toString();
        interval.timeTo = v.value(2).toString();

        intervals << interval;
    }

    return intervals;
}

bool processConfTimePeriodManager_addOrUpdateTimePeriod(
        ConfTimePeriodManager *confTimePeriodManager, const ProcessCommandArgs &p,
        ProcessCommandResult &r)
{
    TimePeriod period = ConfTimePeriodManagerRpc::varListToTimePeriod(p.args);

    const bool ok = confTimePeriodManager->addOrUpdateTimePeriod(period);
    r.args = { period.periodId };
    return ok;
}

bool processConfTimePeriodManager_deleteTimePeriod(ConfTimePeriodManager *confTimePeriodManager,
        const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confTimePeriodManager->deleteTimePeriod(p.args.value(0).toUInt());
}

bool processConfTimePeriodManager_updateTimePeriodName(ConfTimePeriodManager *confTimePeriodManager,
        const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confTimePeriodManager->updateTimePeriodName(
            p.args.value(0).toUInt(), p.args.value(1).toString());
}

bool processConfTimePeriodManager_updateTimePeriodEnabled(
        ConfTimePeriodManager *confTimePeriodManager, const ProcessCommandArgs &p,
        ProcessCommandResult & /*r*/)
{
    return confTimePeriodManager->updateTimePeriodEnabled(
            p.args.value(0).toUInt(), p.args.value(1).toBool());
}

using processConfTimePeriodManager_func = bool (*)(ConfTimePeriodManager *confTimePeriodManager,
        const ProcessCommandArgs &p, ProcessCommandResult &r);

static const processConfTimePeriodManager_func processConfTimePeriodManager_funcList[] = {
    &processConfTimePeriodManager_addOrUpdateTimePeriod, // Rpc_ConfTimePeriodManager_addOrUpdateTimePeriod,
    &processConfTimePeriodManager_deleteTimePeriod, // Rpc_ConfTimePeriodManager_deleteTimePeriod,
    &processConfTimePeriodManager_updateTimePeriodName, // Rpc_ConfTimePeriodManager_updateTimePeriodName,
    &processConfTimePeriodManager_updateTimePeriodEnabled, // Rpc_ConfTimePeriodManager_updateTimePeriodEnabled,
};

inline bool processConfTimePeriodManagerRpcResult(ConfTimePeriodManager *confTimePeriodManager,
        const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    const processConfTimePeriodManager_func func =
            RpcManager::getProcessFunc(p.command, processConfTimePeriodManager_funcList,
                    Control::Rpc_ConfTimePeriodManager_addOrUpdateTimePeriod,
                    Control::Rpc_ConfTimePeriodManager_updateTimePeriodEnabled);

    return func ? func(confTimePeriodManager, p, r) : false;
}

}

ConfTimePeriodManagerRpc::ConfTimePeriodManagerRpc(QObject *parent) : ConfTimePeriodManager(parent)
{
}

bool ConfTimePeriodManagerRpc::addOrUpdateTimePeriod(TimePeriod &period)
{
    QVariantList resArgs;

    if (!rpcManager()->doOnServer(Control::Rpc_ConfTimePeriodManager_addOrUpdateTimePeriod,
                timePeriodToVarList(period), &resArgs))
        return false;

    period.periodId = resArgs.value(0).toUInt();

    return true;
}

bool ConfTimePeriodManagerRpc::deleteTimePeriod(quint8 periodId)
{
    return rpcManager()->doOnServer(
            Control::Rpc_ConfTimePeriodManager_deleteTimePeriod, { periodId });
}

bool ConfTimePeriodManagerRpc::updateTimePeriodName(quint8 periodId, const QString &name)
{
    return rpcManager()->doOnServer(
            Control::Rpc_ConfTimePeriodManager_updateTimePeriodName, { periodId, name });
}

bool ConfTimePeriodManagerRpc::updateTimePeriodEnabled(quint8 periodId, bool enabled)
{
    return rpcManager()->doOnServer(
            Control::Rpc_ConfTimePeriodManager_updateTimePeriodEnabled, { periodId, enabled });
}

QVariantList ConfTimePeriodManagerRpc::timePeriodToVarList(const TimePeriod &period)
{
    return { period.enabled, period.periodId, period.name, period.notes,
        intervalsToVarList(period.intervals) };
}

TimePeriod ConfTimePeriodManagerRpc::varListToTimePeriod(const QVariantList &v)
{
    TimePeriod period;
    period.enabled = v.value(0).toBool();
    period.periodId = v.value(1).toUInt();
    period.name = v.value(2).toString();
    period.notes = v.value(3).toString();
    period.intervals = varListToIntervals(v.value(4).toList());
    return period;
}

bool ConfTimePeriodManagerRpc::processServerCommand(
        const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    auto confTimePeriodManager = Fort::confTimePeriodManager();

    switch (p.command) {
    case Control::Rpc_ConfTimePeriodManager_timePeriodAdded: {
        emit confTimePeriodManager->timePeriodAdded();
        return true;
    }
    case Control::Rpc_ConfTimePeriodManager_timePeriodRemoved: {
        emit confTimePeriodManager->timePeriodRemoved(p.args.value(0).toUInt());
        return true;
    }
    case Control::Rpc_ConfTimePeriodManager_timePeriodUpdated: {
        emit confTimePeriodManager->timePeriodUpdated();
        return true;
    }
    default: {
        r.ok = processConfTimePeriodManagerRpcResult(confTimePeriodManager, p, r);
        r.isSendResult = true;
        return true;
    }
    }
}

void ConfTimePeriodManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto confTimePeriodManager = Fort::confTimePeriodManager();

    connect(confTimePeriodManager, &ConfTimePeriodManager::timePeriodAdded, rpcManager, [=] {
        rpcManager->invokeOnClients(Control::Rpc_ConfTimePeriodManager_timePeriodAdded);
    });
    connect(confTimePeriodManager, &ConfTimePeriodManager::timePeriodRemoved, rpcManager,
            [=](quint8 periodId) {
                rpcManager->invokeOnClients(
                        Control::Rpc_ConfTimePeriodManager_timePeriodRemoved, { periodId });
            });
    connect(confTimePeriodManager, &ConfTimePeriodManager::timePeriodUpdated, rpcManager, [=] {
        rpcManager->invokeOnClients(Control::Rpc_ConfTimePeriodManager_timePeriodUpdated);
    });
}
