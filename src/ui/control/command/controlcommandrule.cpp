#include "controlcommandrule.h"

#include <conf/confrulemanager.h>
#include <conf/rule.h>
#include <fortglobal.h>

#include "controlcommandutil.h"

using namespace Fort;

namespace {

enum RuleAction : qint8 {
    RuleActionInvalid = -1,
    RuleActionOn = 0,
    RuleActionOff,
    RuleActionSetText,
    RuleActionGetText,
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

    if (commandText == "set-text")
        return RuleActionSetText;

    if (commandText == "get-text")
        return RuleActionGetText;

    if (commandText == "report") {
        report = true;
    }

    return RuleActionInvalid;
}

int ruleActionArgsCount(RuleAction ruleAction)
{
    // The text actions have the <file-path> argument
    return (ruleAction >= RuleActionSetText) ? 3 : 2;
}

RuleAction clientRuleAction(const QVariantList &args)
{
    bool report = false;
    const RuleAction ruleAction = ruleActionByText(args.value(0).toString(), report);

    // The running instance reports the invalid arguments
    return (args.size() >= ruleActionArgsCount(ruleAction)) ? ruleAction : RuleActionInvalid;
}

bool loadRuleByName(ProcessCommandResult &r, Rule &rule, const QString &ruleName)
{
    if (!ControlCommandUtil::findRuleByName(ruleName, rule, r.errorMessage)) {
        r.commandResult = Control::CommandResultError;
        return false;
    }

    return true;
}

bool reportCommandRuleAction(ProcessCommandResult &r, const Rule &rule)
{
    const auto ruleAction = rule.enabled ? RuleActionOn : RuleActionOff;

    r.commandResult = Control::CommandResult(Control::CommandResultBase + ruleAction);

    r.errorMessage = ruleActionNames().value(ruleAction);

    return true;
}

bool setRuleText(ProcessCommandResult &r, const Rule &rule, const QString &ruleText)
{
    if (ruleText == rule.ruleText)
        return true;

    if (!ControlCommandUtil::validateRuleText(r, ruleText))
        return false;

    return confRuleManager()->updateRuleText(rule.ruleId, ruleText);
}

bool getRuleText(ProcessCommandResult &r, const Rule &rule)
{
    // The client writes the text to the file
    r.errorMessage = rule.ruleText;

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

    if (ruleAction == RuleActionSetText) {
        return setRuleText(r, rule, args.value(2).toString());
    }

    if (ruleAction == RuleActionGetText) {
        return getRuleText(r, rule);
    }

    return confRuleManager()->updateRuleEnabled(rule.ruleId, ruleAction == RuleActionOn);
}

}

bool ControlCommandRule::prepareClientArgs(QVariantList &args, ProcessCommandResult &r) const
{
    if (!ControlCommandBase::prepareClientArgs(args, r))
        return false;

    // The rule's text is replaced by a non-empty one
    if (clientRuleAction(args) == RuleActionSetText && args[2].toString().isEmpty()) {
        r.errorMessage = "Empty file";
        return false;
    }

    return true;
}

bool ControlCommandRule::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const
{
    bool report = false;
    const RuleAction ruleAction = ruleActionByText(p.args.value(0).toString(), report);

    const bool isValidAction = (ruleAction != RuleActionInvalid || report);
    if (!isValidAction || p.args.size() < ruleActionArgsCount(ruleAction)) {
        r.errorMessage = "Usage: rule on|off|report <rule-name>"
                         " | rule set-text|get-text <rule-name> <file-path>";
        return false;
    }

    if (!ControlCommandUtil::checkCommandActionPassword(r, ruleAction))
        return false;

    return processCommandRuleAction(r, p.args, ruleAction, report);
}

ControlCommandBase::ClientFileArg ControlCommandRule::clientFileArg(const QVariantList &args) const
{
    switch (clientRuleAction(args)) {
    case RuleActionSetText:
        return ClientFileRead;
    case RuleActionGetText:
        return ClientFileWrite;
    default:
        return ClientFileNone;
    }
}
