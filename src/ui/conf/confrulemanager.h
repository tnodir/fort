#ifndef CONFRULEMANAGER_H
#define CONFRULEMANAGER_H

#include <QObject>

#include <conf/rule.h>
#include <util/classhelpers.h>
#include <util/conf/confruleswalker.h>
#include <util/ioc/iocservice.h>

#include "confmanagerbase.h"
#include "presetrulesmanager.h"

class ConfRuleManager : public ConfManagerBase, public ConfRulesWalker, public IocService
{
    Q_OBJECT

public:
    explicit ConfRuleManager(QObject *parent = nullptr);
    CLASS_DELETE_COPY_MOVE(ConfRuleManager)

    void setUp() override;

    QString ruleNameById(quint16 ruleId);

    QVector<Rule> getTrayMenuRules() const;
    QVector<Rule> getRulesByName(const QString &ruleName) const;

    void loadRuleSet(Rule &rule, QStringList *ruleSetNames = nullptr) const;
    void saveRuleSet(Rule &rule);

    int rulesCountByType(Rule::RuleType ruleType);

    // The Global rules of the type are of the max count: no new one
    bool isGlobalRulesCountExceeded(Rule::RuleType ruleType);
    bool checkRuleSetValid(quint16 ruleId, quint16 subRuleId, int extraDepth = 0);

    bool syncPresetRules(PresetRulesManager::SyncMode mode);

    virtual bool addOrUpdateRule(Rule &rule);
    virtual bool deleteRule(quint16 ruleId);
    virtual bool updateRuleName(quint16 ruleId, const QString &ruleName);
    virtual bool updateRuleEnabled(quint16 ruleId, bool enabled);
    virtual bool updateRuleText(quint16 ruleId, const QString &ruleText);
    virtual bool addDefaultPresetRules();

    bool walkRules(WalkRulesArgs &wra, const std::function<walkRulesCallback> &func) const override;

    static void walkRulesMapByStmt(WalkRulesArgs &wra, SqliteStmt &stmt);

    void updateDriverRules();

signals:
    void ruleAdded();
    void ruleRemoved(quint16 ruleId, int appRulesCount);
    void ruleUpdated(quint16 ruleId);
    void trayMenuUpdated();

private:
    friend class PresetRulesManager;

    bool loadRuleById(quint16 ruleId, Rule &rule) const;

    bool doAddOrUpdateRule(Rule &rule, bool &isNew, bool &isTrayMenuUpdated);

    void walkRulesMap(WalkRulesArgs &wra) const;
    bool walkRulesLoop(const std::function<walkRulesCallback> &func) const;
    bool walkGlobalRule(const std::function<walkRulesCallback> &func, quint16 ruleId) const;

    static void fillRule(Rule &rule, const SqliteStmt &stmt);

    bool updateDriverRuleFlag(quint16 ruleId, bool enabled);

    void setupRuleNamesCache();
    void clearRuleNamesCache();

private:
    mutable QHash<quint16, QString> m_ruleNamesCache;
};

#endif // CONFRULEMANAGER_H
