#include "confrulemanagerrpc.h"

#include <conf/rule.h>
#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

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

bool ConfRuleManagerRpc::deleteRule(int ruleId)
{
    return IoC<RpcManager>()->doOnServer(Control::Rpc_ConfRuleManager_deleteRule, { ruleId });
}

bool ConfRuleManagerRpc::updateRuleName(int ruleId, const QString &ruleName)
{
    return IoC<RpcManager>()->doOnServer(
            Control::Rpc_ConfRuleManager_updateRuleName, { ruleId, ruleName });
}

bool ConfRuleManagerRpc::updateRuleEnabled(int ruleId, bool enabled)
{
    return IoC<RpcManager>()->doOnServer(
            Control::Rpc_ConfRuleManager_updateRuleEnabled, { ruleId, enabled });
}

QVariantList ConfRuleManagerRpc::ruleToVarList(const Rule &rule)
{
    return { rule.enabled, rule.blocked, rule.exclusive, rule.ruleId, rule.acceptZones,
        rule.rejectZones, rule.ruleName, rule.notes, rule.ruleText };
}

Rule ConfRuleManagerRpc::varListToRule(const QVariantList &v)
{
    Rule rule;
    rule.enabled = v.value(0).toBool();
    rule.blocked = v.value(1).toBool();
    rule.exclusive = v.value(2).toBool();
    rule.ruleId = v.value(3).toInt();
    rule.acceptZones = v.value(4).toUInt();
    rule.rejectZones = v.value(5).toUInt();
    rule.ruleName = v.value(6).toString();
    rule.notes = v.value(7).toString();
    rule.ruleText = v.value(8).toString();
    return rule;
}

void ConfRuleManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto confRuleManager = IoC<ConfRuleManager>();

    connect(confRuleManager, &ConfRuleManager::ruleAdded, rpcManager,
            [&] { rpcManager->invokeOnClients(Control::Rpc_ConfRuleManager_ruleAdded); });
    connect(confRuleManager, &ConfRuleManager::ruleRemoved, rpcManager, [&](int ruleId) {
        rpcManager->invokeOnClients(Control::Rpc_ConfRuleManager_ruleRemoved, { ruleId });
    });
    connect(confRuleManager, &ConfRuleManager::ruleUpdated, rpcManager,
            [&] { rpcManager->invokeOnClients(Control::Rpc_ConfRuleManager_ruleUpdated); });
}
