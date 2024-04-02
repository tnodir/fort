#include "rulesetmodel.h"

#include <conf/confrulemanager.h>
#include <util/ioc/ioccontainer.h>

#include "rulelistmodel.h"

RuleSetModel::RuleSetModel(QObject *parent) : StringListModel(parent) { }

ConfRuleManager *RuleSetModel::confRuleManager() const
{
    return IoC<ConfRuleManager>();
}

void RuleSetModel::initialize(const RuleRow &ruleRow, const QStringList &ruleSetNames)
{
    setEdited(false);

    m_ruleSet = ruleRow.ruleSet;

    setList(ruleSetNames);
}

void RuleSetModel::addRule(const RuleRow &ruleRow)
{
    if (m_ruleSet.contains(ruleRow.ruleId))
        return;

    m_ruleSet.append(ruleRow.ruleId);

    insert(ruleRow.ruleName);

    setEdited(true);
}
