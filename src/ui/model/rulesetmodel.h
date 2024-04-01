#ifndef RULESETMODEL_H
#define RULESETMODEL_H

#include <conf/rule.h>
#include <util/model/stringlistmodel.h>

class ConfRuleManager;

class RuleSetModel : public StringListModel
{
    Q_OBJECT

public:
    explicit RuleSetModel(QObject *parent = nullptr);

    ConfRuleManager *confRuleManager() const;

    void initialize(const Rule &rule);

private:
    RuleSetList m_ruleSet;
};

#endif // RULESETMODEL_H
