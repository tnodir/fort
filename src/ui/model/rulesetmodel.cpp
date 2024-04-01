#include "rulesetmodel.h"

#include <conf/confrulemanager.h>
#include <util/ioc/ioccontainer.h>

RuleSetModel::RuleSetModel(QObject *parent) : StringListModel(parent) { }

ConfRuleManager *RuleSetModel::confRuleManager() const
{
    return IoC<ConfRuleManager>();
}

void RuleSetModel::initialize(const Rule &rule)
{
    beginResetModel();

    m_ruleSet = rule.ruleSet;

    endResetModel();
}
