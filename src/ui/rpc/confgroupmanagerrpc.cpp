#include "confgroupmanagerrpc.h"

#include <conf/group.h>
#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

namespace {

bool processConfGroupManager_addOrUpdateGroup(
        ConfGroupManager *confGroupManager, const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    Group group = ConfGroupManagerRpc::varListToGroup(p.args);

    const bool ok = confGroupManager->addOrUpdateGroup(group);
    r.args = { group.groupId };
    return ok;
}

bool processConfGroupManager_deleteGroup(ConfGroupManager *confGroupManager,
        const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confGroupManager->deleteGroup(p.args.value(0).toLongLong());
}

bool processConfGroupManager_updateGroupName(ConfGroupManager *confGroupManager,
        const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confGroupManager->updateGroupName(
            p.args.value(0).toLongLong(), p.args.value(1).toString());
}

bool processConfGroupManager_updateGroupEnabled(ConfGroupManager *confGroupManager,
        const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confGroupManager->updateGroupEnabled(
            p.args.value(0).toLongLong(), p.args.value(1).toBool());
}

using processConfGroupManager_func = bool (*)(
        ConfGroupManager *confGroupManager, const ProcessCommandArgs &p, ProcessCommandResult &r);

static const processConfGroupManager_func processConfGroupManager_funcList[] = {
    &processConfGroupManager_addOrUpdateGroup, // Rpc_ConfGroupManager_addOrUpdateGroup,
    &processConfGroupManager_deleteGroup, // Rpc_ConfGroupManager_deleteGroup,
    &processConfGroupManager_updateGroupName, // Rpc_ConfGroupManager_updateGroupName,
    &processConfGroupManager_updateGroupEnabled, // Rpc_ConfGroupManager_updateGroupEnabled,
};

inline bool processConfGroupManagerRpcResult(
        ConfGroupManager *confGroupManager, const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    const processConfGroupManager_func func = RpcManager::getProcessFunc(p.command,
            processConfGroupManager_funcList, Control::Rpc_ConfGroupManager_addOrUpdateGroup,
            Control::Rpc_ConfGroupManager_updateGroupEnabled);

    return func ? func(confGroupManager, p, r) : false;
}

}

ConfGroupManagerRpc::ConfGroupManagerRpc(QObject *parent) : ConfGroupManager(parent) { }

bool ConfGroupManagerRpc::addOrUpdateGroup(Group &group)
{
    QVariantList resArgs;

    if (!IoC<RpcManager>()->doOnServer(
                Control::Rpc_ConfGroupManager_addOrUpdateGroup, groupToVarList(group), &resArgs))
        return false;

    group.groupId = resArgs.value(0).toInt();

    return true;
}

bool ConfGroupManagerRpc::deleteGroup(quint8 groupId)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfGroupManager_deleteGroup, { groupId });
}

bool ConfGroupManagerRpc::updateGroupName(quint8 groupId, const QString &groupName)
{
    return IoC<RpcManager>()->doOnServer(
            Control::Rpc_ConfGroupManager_updateGroupName, { groupId, groupName });
}

bool ConfGroupManagerRpc::updateGroupEnabled(quint8 groupId, bool enabled)
{
    return IoC<RpcManager>()->doOnServer(
            Control::Rpc_ConfGroupManager_updateGroupEnabled, { groupId, enabled });
}

QVariantList ConfGroupManagerRpc::groupToVarList(const Group &group)
{
    return { group.enabled, group.exclusive, group.periodEnabled, group.groupId, group.ruleId,
        group.groupName, group.notes, group.periodFrom, group.periodTo };
}

Group ConfGroupManagerRpc::varListToGroup(const QVariantList &v)
{
    Group group;
    group.enabled = v.value(0).toBool();
    group.exclusive = v.value(1).toBool();
    group.periodEnabled = v.value(2).toBool();
    group.groupId = v.value(3).toInt();
    group.ruleId = v.value(4).toInt();
    group.groupName = v.value(5).toString();
    group.notes = v.value(6).toString();
    group.periodFrom = v.value(7).toString();
    group.periodTo = v.value(8).toString();
    return group;
}

bool ConfGroupManagerRpc::processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    auto confGroupManager = IoC<ConfGroupManager>();

    switch (p.command) {
    case Control::Rpc_ConfGroupManager_groupAdded: {
        emit confGroupManager->groupAdded();
        return true;
    }
    case Control::Rpc_ConfGroupManager_groupRemoved: {
        emit confGroupManager->groupRemoved(p.args.value(0).toInt());
        return true;
    }
    case Control::Rpc_ConfGroupManager_groupUpdated: {
        emit confGroupManager->groupUpdated();
        return true;
    }
    default: {
        r.ok = processConfGroupManagerRpcResult(confGroupManager, p, r);
        r.isSendResult = true;
        return true;
    }
    }
}

void ConfGroupManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto confGroupManager = IoC<ConfGroupManager>();

    connect(confGroupManager, &ConfGroupManager::groupAdded, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_ConfGroupManager_groupAdded); });
    connect(confGroupManager, &ConfGroupManager::groupRemoved, rpcManager, [=](quint8 groupId) {
        rpcManager->invokeOnClients(Control::Rpc_ConfGroupManager_groupRemoved, { groupId });
    });
    connect(confGroupManager, &ConfGroupManager::groupUpdated, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_ConfGroupManager_groupUpdated); });
}
