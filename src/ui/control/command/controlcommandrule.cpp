#include "controlcommandrule.h"

#include <QDir>

#include <conf/confrulemanager.h>
#include <conf/rule.h>
#include <fortglobal.h>
#include <fortsettings.h>
#include <util/conf/confbuffer.h>
#include <util/fileutil.h>

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

QString clientFilePath(const QVariantList &args)
{
    // The working directory is changed to the program's one on startup
    const QDir currentDir(settings()->initialCurrentPath());

    return currentDir.absoluteFilePath(args.value(2).toString());
}

bool readRuleTextFile(ProcessCommandResult &r, const QString &filePath, QString &ruleText)
{
    ruleText = FileUtil::readFile(filePath);

    // A directory or an unreadable file has no text too
    if (ruleText.isEmpty()) {
        r.errorMessage = "Empty or unreadable file";
        return false;
    }

    return true;
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

bool validateRuleText(ProcessCommandResult &r, const QString &ruleText)
{
    if (ruleText.isEmpty())
        return true;

    ConfBuffer confBuf;
    if (!confBuf.validateRuleText(ruleText)) {
        r.errorMessage = confBuf.errorMessage();
        return false;
    }

    return true;
}

bool setRuleText(ProcessCommandResult &r, const Rule &rule, const QString &ruleText)
{
    if (ruleText == rule.ruleText)
        return true;

    if (!validateRuleText(r, ruleText))
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
    if (clientRuleAction(args) != RuleActionSetText)
        return true;

    // Send the file's text instead of its path
    QString ruleText;
    if (!readRuleTextFile(r, clientFilePath(args), ruleText))
        return false;

    args[2] = ruleText;

    return true;
}

bool ControlCommandRule::processClientResult(
        const QVariantList &args, ProcessCommandResult &r) const
{
    if (clientRuleAction(args) != RuleActionGetText)
        return true;

    if (r.commandResult == Control::CommandResultError)
        return true;

    if (!FileUtil::writeFile(clientFilePath(args), r.errorMessage)) {
        r.errorMessage = "Cannot write the file";
        return false;
    }

    r.errorMessage.clear();

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

    if (!checkCommandActionPassword(r, ruleAction))
        return false;

    const bool ok = processCommandRuleAction(r, p.args, ruleAction, report);

    uncheckCommandActionPassword();

    return ok;
}
