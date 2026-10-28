#include "controlcommandrule.h"

#include <conf/confrulemanager.h>
#include <conf/rule.h>
#include <fortglobal.h>
#include <util/net/actionrange.h>

#include "controlcommandutil.h"

using namespace Fort;

namespace {

enum RuleAction : qint8 {
    RuleActionInvalid = -1,
    RuleActionOn = 0,
    RuleActionOff,
    RuleActionDel,
    RuleActionGet,
    RuleActionAdd,
    RuleActionSet,
    RuleActionSetText,
    RuleActionGetText,
};

QStringList ruleActionNames()
{
    // Sync with enum RuleAction
    return { "on", "off", "del", "get", "add", "set", "set-text", "get-text" };
}

QStringList ruleTypeNames()
{
    // Sync with enum Rule::RuleType
    return { "app", "global-before", "global-after", "preset" };
}

RuleAction ruleActionByText(const QString &commandText, bool &report)
{
    report = (commandText == "report");

    return RuleAction(ruleActionNames().indexOf(commandText));
}

int ruleActionArgsCount(RuleAction ruleAction)
{
    // The add and set actions have the <file-path> or the fields' arguments
    return (ruleAction >= RuleActionAdd) ? 3 : 2;
}

RuleAction clientRuleAction(const QVariantList &args)
{
    bool report = false;
    const RuleAction ruleAction = ruleActionByText(args.value(0).toString(), report);

    // The running instance reports the invalid arguments
    return (args.size() >= ruleActionArgsCount(ruleAction)) ? ruleAction : RuleActionInvalid;
}

QString argRuleName(const QVariantList &args)
{
    return args.value(1).toString();
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

QString actionTypeName(quint8 actionTypeId)
{
    return ActionRange::actionTypeName(actionTypeId).toLower();
}

QVariantMap ruleFields(const Rule &rule)
{
    return {
        { "enabled", rule.enabled },
        { "trayMenu", rule.trayMenu },
        { "action", actionTypeName(rule.actionTypeId()) },
        { "exclusive", rule.exclusive },
        { "inlineZones", rule.inlineZones },
        { "terminate", rule.terminate },
        { "terminateAction", actionTypeName(rule.terminateActionTypeId()) },
        { "terminateAlert", rule.terminateAlert },
        { "logAllowedConn", rule.logAllowedConn },
        { "logBlockedConn", rule.logBlockedConn },
        { "periodEnabled", rule.periodEnabled },
        { "type", ruleTypeNames().value(rule.ruleType) },
        { "period", QVariant::fromValue(rule.periodId) },
        { "acceptZones", ControlCommandUtil::maskIds(rule.zones.accept_mask) },
        { "rejectZones", ControlCommandUtil::maskIds(rule.zones.reject_mask) },
        { "name", rule.ruleName },
        { "notes", rule.notes },
    };
}

void fillRuleFlags(Rule &rule, const QVariantMap &fields)
{
    rule.enabled = fields["enabled"].toBool();
    rule.trayMenu = fields["trayMenu"].toBool();
    rule.exclusive = fields["exclusive"].toBool();
    rule.inlineZones = fields["inlineZones"].toBool();
    rule.terminate = fields["terminate"].toBool();
    rule.terminateAlert = fields["terminateAlert"].toBool();
    rule.logAllowedConn = fields["logAllowedConn"].toBool();
    rule.logBlockedConn = fields["logBlockedConn"].toBool();
    rule.periodEnabled = fields["periodEnabled"].toBool();
}

void fillRuleIds(Rule &rule, const QVariantMap &fields)
{
    rule.periodId = fields["period"].toUInt();
    rule.zones.accept_mask = ControlCommandUtil::idsMask(fields["acceptZones"]);
    rule.zones.reject_mask = ControlCommandUtil::idsMask(fields["rejectZones"]);
}

// The field's action of the allowed ones, 0 - invalid
quint8 fieldActionTypeId(const QVariantMap &fields, const QString &key, quint8 actionTypesMask,
        QString &errorMessage)
{
    const QString text = fields[key].toString();
    const quint8 actionTypeId = ActionRange::actionTypeIdByName(text) & actionTypesMask;

    if (actionTypeId == 0) {
        errorMessage = ControlCommandUtil::invalidValueText(key, text);
    }

    return actionTypeId;
}

bool fillRuleActions(ProcessCommandResult &r, Rule &rule, const QVariantMap &fields)
{
    constexpr quint8 actionTypesMask = FORT_RULE_FILTER_ACTION_ALLOW | FORT_RULE_FILTER_ACTION_BLOCK
            | FORT_RULE_FILTER_ACTION_ASK;
    constexpr quint8 terminateActionTypesMask = actionTypesMask | FORT_RULE_FILTER_ACTION_DROP;

    const quint8 actionTypeId =
            fieldActionTypeId(fields, "action", actionTypesMask, r.errorMessage);
    if (actionTypeId == 0)
        return false;

    const quint8 terminateActionTypeId =
            fieldActionTypeId(fields, "terminateAction", terminateActionTypesMask, r.errorMessage);
    if (terminateActionTypeId == 0)
        return false;

    rule.setActionTypeId(actionTypeId);
    rule.setTerminateActionTypeId(terminateActionTypeId);

    return true;
}

bool checkRuleNameUnused(ProcessCommandResult &r, const QString &ruleName)
{
    if (!confRuleManager()->getRulesByName(ruleName).isEmpty()) {
        r.errorMessage = "Rule already exists";
        return false;
    }

    return true;
}

bool fillRuleName(ProcessCommandResult &r, Rule &rule, const QString &ruleName)
{
    if (ruleName == rule.ruleName)
        return true;

    if (ruleName.isEmpty()) {
        r.errorMessage = ControlCommandUtil::invalidValueText("name", ruleName);
        return false;
    }

    if (!checkRuleNameUnused(r, ruleName))
        return false;

    rule.ruleName = ruleName;

    return true;
}

bool fillRuleType(ProcessCommandResult &r, Rule &rule, const QVariantMap &fields)
{
    const auto ruleType = Rule::RuleType(
            ControlCommandUtil::nameIndex(fields, "type", ruleTypeNames(), r.errorMessage));

    if (ruleType == Rule::RuleNone)
        return false;

    if (ruleType != rule.ruleType && confRuleManager()->isGlobalRulesCountExceeded(ruleType)) {
        r.errorMessage = "Global rules count exceeded";
        return false;
    }

    rule.ruleType = ruleType;

    return true;
}

// The fields to check
bool fillRuleChecked(ProcessCommandResult &r, Rule &rule, const QVariantMap &fields)
{
    if (!fillRuleActions(r, rule, fields))
        return false;

    if (!fillRuleType(r, rule, fields))
        return false;

    return fillRuleName(r, rule, fields["name"].toString());
}

bool setRuleFields(ProcessCommandResult &r, Rule &rule, const QVariantList &args, int index)
{
    QVariantMap fields = ruleFields(rule);

    if (!ControlCommandUtil::parseFieldArgs(args, index, fields, r.errorMessage))
        return false;

    if (!fillRuleChecked(r, rule, fields))
        return false;

    fillRuleFlags(rule, fields);
    fillRuleIds(rule, fields);

    rule.notes = fields["notes"].toString();

    return confRuleManager()->addOrUpdateRule(rule);
}

bool getRuleFields(ProcessCommandResult &r, const Rule &rule)
{
    r.errorMessage = ControlCommandUtil::fieldsToTextWithId(
            ruleFields(rule), QVariant::fromValue(rule.ruleId));

    return true;
}

bool addRule(ProcessCommandResult &r, const QVariantList &args)
{
    const QString ruleName = argRuleName(args);

    if (!checkRuleNameUnused(r, ruleName))
        return false;

    Rule rule;
    rule.ruleName = ruleName;
    rule.ruleText = args.value(2).toString();

    if (!ControlCommandUtil::validateRuleText(r, rule.ruleText))
        return false;

    if (!setRuleFields(r, rule, args, 3))
        return false;

    r.errorMessage = QString::number(rule.ruleId);

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

bool processCommandRuleOfName(
        ProcessCommandResult &r, Rule &rule, const QVariantList &args, RuleAction ruleAction)
{
    switch (ruleAction) {
    case RuleActionDel: {
        return confRuleManager()->deleteRule(rule.ruleId);
    }
    case RuleActionGet: {
        return getRuleFields(r, rule);
    }
    case RuleActionSet: {
        return setRuleFields(r, rule, args, 2);
    }
    case RuleActionSetText: {
        return setRuleText(r, rule, args.value(2).toString());
    }
    case RuleActionGetText: {
        return getRuleText(r, rule);
    }
    default: // On or Off
        return confRuleManager()->updateRuleEnabled(rule.ruleId, ruleAction == RuleActionOn);
    }
}

bool processCommandRuleAction(
        ProcessCommandResult &r, const QVariantList &args, RuleAction ruleAction, bool report)
{
    if (ruleAction == RuleActionAdd) {
        return addRule(r, args);
    }

    Rule rule;

    if (!loadRuleByName(r, rule, argRuleName(args)))
        return true;

    if (report) {
        return reportCommandRuleAction(r, rule);
    }

    return processCommandRuleOfName(r, rule, args, ruleAction);
}

}

bool ControlCommandRule::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const
{
    bool report = false;
    const RuleAction ruleAction = ruleActionByText(p.args.value(0).toString(), report);

    const bool isValidAction = (ruleAction != RuleActionInvalid || report);
    if (!isValidAction || p.args.size() < ruleActionArgsCount(ruleAction)) {
        r.errorMessage = "Usage: rule on|off|report|get|del <rule-name>"
                         " | rule add <rule-name> <file-path> [<key>=<value>...]"
                         " | rule set <rule-name> <key>=<value>..."
                         " | rule set-text|get-text <rule-name> <file-path>";
        return false;
    }

    if (argRuleName(p.args).isEmpty()) {
        r.errorMessage = "<rule-name> is empty";
        return false;
    }

    if (!ControlCommandUtil::checkCommandActionPassword(r, ruleAction))
        return false;

    return processCommandRuleAction(r, p.args, ruleAction, report);
}

ControlCommandBase::ClientFileArg ControlCommandRule::clientFileArg(const QVariantList &args) const
{
    switch (clientRuleAction(args)) {
    case RuleActionAdd:
    case RuleActionSetText:
        return ClientFileRead;
    case RuleActionGetText:
        return ClientFileWrite;
    default:
        return ClientFileNone;
    }
}
