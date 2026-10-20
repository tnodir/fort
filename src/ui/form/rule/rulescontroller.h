#ifndef RULESCONTROLLER_H
#define RULESCONTROLLER_H

#include <form/basecontroller.h>

class Rule;
class RuleListModel;

class RulesController : public BaseController
{
    Q_OBJECT

public:
    explicit RulesController(QObject *parent = nullptr);

    RuleListModel *ruleListModel() const;

public slots:
    bool addOrUpdateRule(Rule &rule);
    void deleteRule(int ruleId);
    bool updateRuleName(int ruleId, const QString &ruleName);
    void addDefaultPresetRules();

private:
    RuleListModel *m_ruleListModel = nullptr;
};

#endif // RULESCONTROLLER_H
