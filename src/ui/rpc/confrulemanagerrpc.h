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
    bool deleteRule(quint16 ruleId) override;
    bool updateRuleName(quint16 ruleId, const QString &ruleName) override;
    bool updateRuleEnabled(quint16 ruleId, bool enabled) override;

    static QVariantList ruleToVarList(const Rule &rule);
    static Rule varListToRule(const QVariantList &v);

    static bool processServerCommand(
            const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult);

    static void setupServerSignals(RpcManager *rpcManager);
};

#endif // CONFRULEMANAGERRPC_H
