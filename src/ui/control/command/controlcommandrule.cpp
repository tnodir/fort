#include "controlcommandrule.h"

#include <conf/confrulemanager.h>
#include <conf/rule.h>
#include <fortglobal.h>

using namespace Fort;

namespace {

enum RuleAction : qint8 {
    RuleActionInvalid = -1,
    RuleActionOn = 0,
    RuleActionOff,
};

QStringList ruleActionNames()
{
    // Sync with enum RuleAction
    return { "on", "off" };
}

RuleAction ruleActionByText(const QString &commandText, bool &report)
{
    if (commandText == "on")
        return RuleActionOn;

    if (commandText == "off")
        return RuleActionOff;

    if (commandText == "report") {
        report = true;
    }

    return RuleActionInvalid;
}

bool loadRuleByName(ProcessCommandResult &r, Rule &rule, const QString &ruleName)
{
    const auto ruleList = confRuleManager()->getRulesByName(ruleName);

    if (ruleList.size() != 1) {
        r.commandResult = Control::CommandResultError;
        r.errorMessage = ruleList.isEmpty() ? "Rule not found" : "Rule name is ambiguous";
        return false;
    }

    rule = ruleList.first();

    return true;
}

bool reportCommandRuleAction(ProcessCommandResult &r, const Rule &rule)
{
    const auto ruleAction = rule.enabled ? RuleActionOn : RuleActionOff;

    r.commandResult = Control::CommandResult(Control::CommandResultBase + ruleAction);

    r.errorMessage = ruleActionNames().value(ruleAction);

    return true;
}

bool processCommandRuleAction(
        ProcessCommandResult &r, const QVariantList &args, RuleAction ruleAction, bool report)
{
    Rule rule;

    if (!loadRuleByName(r, rule, args.value(1).toString()))
        return true;

    if (report) {
        return reportCommandRuleAction(r, rule);
    }

    return confRuleManager()->updateRuleEnabled(rule.ruleId, ruleAction == RuleActionOn);
}

}

bool ControlCommandRule::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const
{
    bool report = false;
    const RuleAction ruleAction = ruleActionByText(p.args.value(0).toString(), report);

    const bool isValidAction = (ruleAction != RuleActionInvalid || report);
    if (!isValidAction || p.args.size() < 2) {
        r.errorMessage = "Usage: rule on|off|report <rule-name>";
        return false;
    }

    if (!checkCommandActionPassword(r, ruleAction))
        return false;

    const bool ok = processCommandRuleAction(r, p.args, ruleAction, report);

    uncheckCommandActionPassword();

    return ok;
}
