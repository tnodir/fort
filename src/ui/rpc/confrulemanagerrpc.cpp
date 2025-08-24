#include "confrulemanagerrpc.h"

#include <conf/rule.h>
#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>
#include <util/variantutil.h>

namespace {

bool processConfRuleManager_addOrUpdateRule(
        ConfRuleManager *confRuleManager, const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    Rule rule = ConfRuleManagerRpc::varListToRule(p.args);

    const bool ok = confRuleManager->addOrUpdateRule(rule);
    r.args = { rule.ruleId };
    return ok;
}

bool processConfRuleManager_deleteRule(
        ConfRuleManager *confRuleManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confRuleManager->deleteRule(p.args.value(0).toLongLong());
}

bool processConfRuleManager_updateRuleName(
        ConfRuleManager *confRuleManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confRuleManager->updateRuleName(
            p.args.value(0).toLongLong(), p.args.value(1).toString());
}

bool processConfRuleManager_updateRuleEnabled(
        ConfRuleManager *confRuleManager, const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    return confRuleManager->updateRuleEnabled(
            p.args.value(0).toLongLong(), p.args.value(1).toBool());
}

using processConfRuleManager_func = bool (*)(
        ConfRuleManager *confRuleManager, const ProcessCommandArgs &p, ProcessCommandResult &r);

static const processConfRuleManager_func processConfRuleManager_funcList[] = {
    &processConfRuleManager_addOrUpdateRule, // Rpc_ConfRuleManager_addOrUpdateRule,
    &processConfRuleManager_deleteRule, // Rpc_ConfRuleManager_deleteRule,
    &processConfRuleManager_updateRuleName, // Rpc_ConfRuleManager_updateRuleName,
    &processConfRuleManager_updateRuleEnabled, // Rpc_ConfRuleManager_updateRuleEnabled,
};

inline bool processConfRuleManagerRpcResult(
        ConfRuleManager *confRuleManager, const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    const processConfRuleManager_func func = RpcManager::getProcessFunc(p.command,
            processConfRuleManager_funcList, Control::Rpc_ConfRuleManager_addOrUpdateRule,
            Control::Rpc_ConfRuleManager_updateRuleEnabled);

    return func ? func(confRuleManager, p, r) : false;
}

}

ConfRuleManagerRpc::ConfRuleManagerRpc(QObject *parent) : ConfRuleManager(parent) { }

bool ConfRuleManagerRpc::addOrUpdateRule(Rule &rule)
{
    QVariantList resArgs;

    if (!IoC<RpcManager>()->doOnServer(
                Control::Rpc_ConfRuleManager_addOrUpdateRule, ruleToVarList(rule), &resArgs))
        return false;

    rule.ruleId = resArgs.value(0).toInt();

    return true;
}

bool ConfRuleManagerRpc::deleteRule(quint16 ruleId)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfRuleManager_deleteRule, { ruleId });
}

bool ConfRuleManagerRpc::updateRuleName(quint16 ruleId, const QString &ruleName)
{
    return IoC<RpcManager>()->doOnServer(
            Control::Rpc_ConfRuleManager_updateRuleName, { ruleId, ruleName });
}

bool ConfRuleManagerRpc::updateRuleEnabled(quint16 ruleId, bool enabled)
{
    return IoC<RpcManager>()->doOnServer(
            Control::Rpc_ConfRuleManager_updateRuleEnabled, { ruleId, enabled });
}

QVariantList ConfRuleManagerRpc::ruleToVarList(const Rule &rule)
{
    QVariantList ruleSetList;
    VariantUtil::vectorToList(rule.ruleSet, ruleSetList);

    return { rule.enabled, rule.trayMenu, rule.blocked, rule.exclusive, rule.inlineZones,
        rule.terminate, rule.terminateBlocked, rule.logAllowedConn, rule.logBlockedConn,
        rule.ruleSetEdited, rule.ruleType, rule.ruleId, rule.zones.accept_mask,
        rule.zones.reject_mask, rule.ruleName, rule.notes, rule.ruleText, ruleSetList };
}

Rule ConfRuleManagerRpc::varListToRule(const QVariantList &v)
{
    Rule rule;
    rule.enabled = v.value(0).toBool();
    rule.trayMenu = v.value(1).toBool();
    rule.blocked = v.value(2).toBool();
    rule.exclusive = v.value(3).toBool();
    rule.inlineZones = v.value(4).toBool();
    rule.terminate = v.value(5).toBool();
    rule.terminateBlocked = v.value(6).toBool();
    rule.logAllowedConn = v.value(7).toBool();
    rule.logBlockedConn = v.value(8).toBool();
    rule.ruleSetEdited = v.value(9).toBool();
    rule.ruleType = Rule::RuleType(v.value(10).toInt());
    rule.ruleId = v.value(11).toInt();
    rule.zones.accept_mask = v.value(12).toUInt();
    rule.zones.reject_mask = v.value(13).toUInt();
    rule.ruleName = v.value(14).toString();
    rule.notes = v.value(15).toString();
    rule.ruleText = v.value(16).toString();
    VariantUtil::listToVector(v.value(17).toList(), rule.ruleSet);
    return rule;
}

bool ConfRuleManagerRpc::processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    auto confRuleManager = IoC<ConfRuleManager>();

    switch (p.command) {
    case Control::Rpc_ConfRuleManager_ruleAdded: {
        emit confRuleManager->ruleAdded();
        return true;
    }
    case Control::Rpc_ConfRuleManager_ruleRemoved: {
        emit confRuleManager->ruleRemoved(p.args.value(0).toInt(), p.args.value(1).toInt());
        return true;
    }
    case Control::Rpc_ConfRuleManager_ruleUpdated: {
        emit confRuleManager->ruleUpdated(p.args.value(0).toInt());
        return true;
    }
    case Control::Rpc_ConfRuleManager_trayMenuUpdated: {
        emit confRuleManager->trayMenuUpdated();
        return true;
    }
    default: {
        r.ok = processConfRuleManagerRpcResult(confRuleManager, p, r);
        r.isSendResult = true;
        return true;
    }
    }
}

void ConfRuleManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto confRuleManager = IoC<ConfRuleManager>();

    connect(confRuleManager, &ConfRuleManager::ruleAdded, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_ConfRuleManager_ruleAdded); });
    connect(confRuleManager, &ConfRuleManager::ruleRemoved, rpcManager,
            [=](quint16 ruleId, int appRulesCount) {
                rpcManager->invokeOnClients(
                        Control::Rpc_ConfRuleManager_ruleRemoved, { ruleId, appRulesCount });
            });
    connect(confRuleManager, &ConfRuleManager::ruleUpdated, rpcManager, [=](quint16 ruleId) {
        rpcManager->invokeOnClients(Control::Rpc_ConfRuleManager_ruleUpdated, { ruleId });
    });
    connect(confRuleManager, &ConfRuleManager::trayMenuUpdated, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_ConfRuleManager_trayMenuUpdated); });
}
