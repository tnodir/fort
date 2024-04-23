#include "confrulemanagerrpc.h"

#include <conf/rule.h>
#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>
#include <util/variantutil.h>

namespace {

bool processConfRuleManager_addOrUpdateRule(
        ConfRuleManager *confRuleManager, const ProcessCommandArgs &p, QVariantList &resArgs)
{
    Rule rule = ConfRuleManagerRpc::varListToRule(p.args);

    const bool ok = confRuleManager->addOrUpdateRule(rule);
    resArgs = { rule.ruleId };
    return ok;
}

bool processConfRuleManager_deleteRule(
        ConfRuleManager *confRuleManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confRuleManager->deleteRule(p.args.value(0).toLongLong());
}

bool processConfRuleManager_updateRuleName(
        ConfRuleManager *confRuleManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confRuleManager->updateRuleName(
            p.args.value(0).toLongLong(), p.args.value(1).toString());
}

bool processConfRuleManager_updateRuleEnabled(
        ConfRuleManager *confRuleManager, const ProcessCommandArgs &p, QVariantList & /*resArgs*/)
{
    return confRuleManager->updateRuleEnabled(
            p.args.value(0).toLongLong(), p.args.value(1).toBool());
}

using processConfRuleManager_func = bool (*)(
        ConfRuleManager *confRuleManager, const ProcessCommandArgs &p, QVariantList &resArgs);

static processConfRuleManager_func processConfRuleManager_funcList[] = {
    &processConfRuleManager_addOrUpdateRule, // Rpc_ConfRuleManager_addOrUpdateRule,
    &processConfRuleManager_deleteRule, // Rpc_ConfRuleManager_deleteRule,
    &processConfRuleManager_updateRuleName, // Rpc_ConfRuleManager_updateRuleName,
    &processConfRuleManager_updateRuleEnabled, // Rpc_ConfRuleManager_updateRuleEnabled,
};

inline bool processConfRuleManagerRpcResult(
        ConfRuleManager *confRuleManager, const ProcessCommandArgs &p, QVariantList &resArgs)
{
    const processConfRuleManager_func func = RpcManager::getProcessFunc(p.command,
            processConfRuleManager_funcList, Control::Rpc_ConfRuleManager_addOrUpdateRule,
            Control::Rpc_ConfRuleManager_updateRuleEnabled);

    return func ? func(confRuleManager, p, resArgs) : false;
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
    QVariantList ruleSetList;
    VariantUtil::vectorToList(rule.ruleSet, ruleSetList);

    return { rule.enabled, rule.blocked, rule.exclusive, rule.ruleSetEdited, rule.ruleType,
        rule.ruleId, rule.acceptZones, rule.rejectZones, rule.ruleName, rule.notes, rule.ruleText,
        ruleSetList };
}

Rule ConfRuleManagerRpc::varListToRule(const QVariantList &v)
{
    Rule rule;
    rule.enabled = v.value(0).toBool();
    rule.blocked = v.value(1).toBool();
    rule.exclusive = v.value(2).toBool();
    rule.ruleSetEdited = v.value(3).toBool();
    rule.ruleType = Rule::RuleType(v.value(4).toInt());
    rule.ruleId = v.value(5).toInt();
    rule.acceptZones = v.value(6).toUInt();
    rule.rejectZones = v.value(7).toUInt();
    rule.ruleName = v.value(8).toString();
    rule.notes = v.value(9).toString();
    rule.ruleText = v.value(10).toString();
    VariantUtil::listToVector(v.value(11).toList(), rule.ruleSet);
    return rule;
}

bool ConfRuleManagerRpc::processServerCommand(
        const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult)
{
    auto confRuleManager = IoC<ConfRuleManager>();

    switch (p.command) {
    case Control::Rpc_ConfRuleManager_ruleAdded: {
        emit confRuleManager->ruleAdded();
        return true;
    }
    case Control::Rpc_ConfRuleManager_ruleRemoved: {
        emit confRuleManager->ruleRemoved(p.args.value(0).toInt());
        return true;
    }
    case Control::Rpc_ConfRuleManager_ruleUpdated: {
        emit confRuleManager->ruleUpdated();
        return true;
    }
    default: {
        ok = processConfRuleManagerRpcResult(confRuleManager, p, resArgs);
        isSendResult = true;
        return true;
    }
    }
}

void ConfRuleManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto confRuleManager = IoC<ConfRuleManager>();

    connect(confRuleManager, &ConfRuleManager::ruleAdded, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_ConfRuleManager_ruleAdded); });
    connect(confRuleManager, &ConfRuleManager::ruleRemoved, rpcManager, [=](int ruleId) {
        rpcManager->invokeOnClients(Control::Rpc_ConfRuleManager_ruleRemoved, { ruleId });
    });
    connect(confRuleManager, &ConfRuleManager::ruleUpdated, rpcManager,
            [=] { rpcManager->invokeOnClients(Control::Rpc_ConfRuleManager_ruleUpdated); });
}
