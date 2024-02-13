#ifndef CONFRULEMANAGERRPC_H
#define CONFRULEMANAGERRPC_H

#include <conf/confrulemanager.h>

class RpcManager;

struct ProcessCommandArgs;

class ConfRuleManagerRpc : public ConfRuleManager
{
    Q_OBJECT

public:
    explicit ConfRuleManagerRpc(QObject *parent = nullptr);

    bool addOrUpdateRule(Rule &rule) override;
    bool deleteRule(int ruleId) override;
    bool updateRuleName(int ruleId, const QString &ruleName) override;
    bool updateRuleEnabled(int ruleId, bool enabled) override;

    static QVariantList ruleToVarList(const Rule &rule);
    static Rule varListToRule(const QVariantList &v);

    static bool processServerCommand(
            const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult);

    static void setupServerSignals(RpcManager *rpcManager);
};

#endif // CONFRULEMANAGERRPC_H
