#include "rulesetmodel.h"

#include <QLoggingCategory>

#include <conf/confrulemanager.h>
#include <util/ioc/ioccontainer.h>

#include "rulelistmodel.h"

namespace {

const QLoggingCategory LC("model.ruleSet");

}

RuleSetModel::RuleSetModel(QObject *parent) : StringListModel(parent) { }

ConfRuleManager *RuleSetModel::confRuleManager() const
{
    return IoC<ConfRuleManager>();
}

void RuleSetModel::initialize(const RuleRow &ruleRow, const QStringList &ruleSetNames)
{
    setEdited(false);

    m_ruleId = ruleRow.ruleId;
    m_ruleSet = ruleRow.ruleSet;

    setList(ruleSetNames);
}

void RuleSetModel::addRule(const RuleRow &ruleRow)
{
    const int subRuleId = ruleRow.ruleId;

    if (m_ruleSet.contains(subRuleId)) {
        qCDebug(LC) << "Sub-Rule already exists";
        return;
    }

    if (confRuleManager()->checkRuleSetLoop(m_ruleId, subRuleId)) {
        qCDebug(LC) << "Rule Set loop detected";
        return;
    }

    m_ruleSet.append(subRuleId);

    insert(ruleRow.ruleName);

    setEdited(true);
}

void RuleSetModel::remove(int row)
{
    row = adjustRow(row);

    m_ruleSet.remove(row);

    StringListModel::remove(row);

    setEdited(true);
}

void RuleSetModel::move(int fromRow, int toRow)
{
    if (!StringListModel::canMove(fromRow, toRow))
        return;

    m_ruleSet.move(fromRow, toRow);

    StringListModel::move(fromRow, toRow);

    setEdited(true);
}
