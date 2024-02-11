#include "rulescontroller.h"

#include <conf/confrulemanager.h>
#include <manager/windowmanager.h>
#include <model/rulelistmodel.h>
#include <util/ioc/ioccontainer.h>

namespace {

void showErrorMessage(const QString &errorMessage)
{
    IoC<WindowManager>()->showErrorBox(
            errorMessage, RulesController::tr("Rule Configuration Error"));
}

}

RulesController::RulesController(QObject *parent) :
    BaseController(parent), m_ruleListModel(new RuleListModel(this))
{
}

RuleListModel *RulesController::ruleListModel() const
{
    return m_ruleListModel;
}

bool RulesController::addOrUpdateRule(Rule &rule)
{
    if (!confRuleManager()->addOrUpdateRule(rule)) {
        showErrorMessage(tr("Cannot edit Rule"));
        return false;
    }
    return true;
}

void RulesController::deleteRule(int ruleId)
{
    if (!confRuleManager()->deleteRule(ruleId)) {
        showErrorMessage(tr("Cannot delete Rule"));
    }
}

bool RulesController::updateRuleName(int ruleId, const QString &ruleName)
{
    if (!confRuleManager()->updateRuleName(ruleId, ruleName)) {
        showErrorMessage(tr("Cannot update Rule's name"));
        return false;
    }
    return true;
}
