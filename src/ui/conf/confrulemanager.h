#ifndef CONFRULEMANAGER_H
#define CONFRULEMANAGER_H

#include <QObject>

#include <sqlite/sqlitetypes.h>

#include <conf/rule.h>
#include <util/classhelpers.h>
#include <util/conf/confruleswalker.h>
#include <util/ioc/iocservice.h>

class ConfManager;

class ConfRuleManager : public QObject, public ConfRulesWalker, public IocService
{
    Q_OBJECT

public:
    explicit ConfRuleManager(QObject *parent = nullptr);
    CLASS_DELETE_COPY_MOVE(ConfRuleManager)

    ConfManager *confManager() const;
    SqliteDb *sqliteDb() const;

    void setUp() override;

    void loadRuleSet(Rule &rule, QStringList &ruleSetNames);
    void saveRuleSet(Rule &rule);

    int rulesCountByType(Rule::RuleType ruleType);
    bool checkRuleSetValid(int ruleId, int subRuleId, int extraDepth = 0);

    virtual bool addOrUpdateRule(Rule &rule);
    virtual bool deleteRule(int ruleId);
    virtual bool updateRuleName(int ruleId, const QString &ruleName);
    virtual bool updateRuleEnabled(int ruleId, bool enabled);

    bool walkRules(ruleset_map_t &ruleSetMap, ruleid_arr_t &ruleIds, int &maxRuleId,
            const std::function<walkRulesCallback> &func) const override;

    void updateDriverRules();

signals:
    void ruleAdded();
    void ruleRemoved(int ruleId);
    void ruleUpdated();

private:
    void walkRulesMap(ruleset_map_t &ruleSetMap, ruleid_arr_t &ruleIds) const;
    bool walkRulesLoop(const std::function<walkRulesCallback> &func) const;

    static void fillRule(Rule &rule, const SqliteStmt &stmt);

    bool updateDriverRuleFlag(int ruleId, bool enabled);

    bool beginTransaction();
    void commitTransaction(bool &ok);

private:
    ConfManager *m_confManager = nullptr;
};

#endif // CONFRULEMANAGER_H
