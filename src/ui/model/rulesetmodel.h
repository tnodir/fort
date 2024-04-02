#ifndef RULESETMODEL_H
#define RULESETMODEL_H

#include <conf/rule.h>
#include <util/model/stringlistmodel.h>

class ConfRuleManager;

struct RuleRow;

class RuleSetModel : public StringListModel
{
    Q_OBJECT

public:
    explicit RuleSetModel(QObject *parent = nullptr);

    bool edited() const { return m_edited; }
    void setEdited(bool v) { m_edited = v; }

    const RuleSetList &ruleSet() const { return m_ruleSet; }

    ConfRuleManager *confRuleManager() const;

    void initialize(const RuleRow &ruleRow, const QStringList &ruleSetNames);

public slots:
    void addRule(const RuleRow &ruleRow);

private:
    bool m_edited = false;

    int m_ruleId = 0;

    RuleSetList m_ruleSet;
};

#endif // RULESETMODEL_H
