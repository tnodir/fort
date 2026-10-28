#include "controlcommandprog.h"

#include <algorithm>

#include <conf/app.h>
#include <conf/confappmanager.h>
#include <conf/confrulemanager.h>
#include <conf/rule.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <util/conf/confutil.h>

#include "controlcommandutil.h"

using namespace Fort;

namespace {

enum ProgAction : qint8 {
    ProgActionInvalid = -1,
    ProgActionAllow = 0,
    ProgActionBlock,
    ProgActionKill,
    ProgActionAsk,
    ProgActionAdd,
    ProgActionDel,
    ProgActionGet,
    ProgActionSet,
    ProgActionSetFilters,
    ProgActionGetFilters,
};

QStringList progActionNames()
{
    // Sync with enum ProgAction
    return { "allow", "block", "kill", "ask", "add", "del", "get", "set", "set-filters",
        "get-filters" };
}

// Sync with enum ProgAction
inline constexpr App::Action appActions[] = { App::ActionAllow, App::ActionBlock,
    App::ActionKillProcess, App::ActionAsk };

App::Action appActionByProg(ProgAction progAction)
{
    return appActions[progAction];
}

ProgAction progActionByApp(const App &app)
{
    const auto it = std::find(std::begin(appActions), std::end(appActions), app.action());

    return ProgAction(it - std::begin(appActions));
}

ProgAction progActionByText(const QString &commandText, bool &report)
{
    report = (commandText == "report");

    return ProgAction(progActionNames().indexOf(commandText));
}

int progActionArgsCount(ProgAction progAction)
{
    // The set actions have the fields or the <file-path> argument
    return (progAction >= ProgActionSet) ? 3 : 1;
}

ProgAction clientProgAction(const QVariantList &args)
{
    bool report = false;
    const ProgAction progAction = progActionByText(args.value(0).toString(), report);

    // The running instance reports the invalid arguments
    return (args.size() >= progActionArgsCount(progAction)) ? progAction : ProgActionInvalid;
}

QString argAppPath(const QVariantList &args)
{
    return args.value(1).toString().trimmed();
}

bool loadAppByPath(ProcessCommandResult &r, App &app, const QString &appPath)
{
    app = confAppManager()->appByPath(appPath);

    if (!app.isValid()) {
        ControlCommandUtil::setErrorResult(r, "App not found");
        return false;
    }

    return true;
}

bool reportCommandProgAction(ProcessCommandResult &r, const App &app)
{
    const auto progAction = progActionByApp(app);

    r.commandResult = Control::CommandResult(Control::CommandResultBase + progAction);

    r.errorMessage = progActionNames().value(progAction);

    return true;
}

QVariantMap appFields(const App &app)
{
    return {
        { "action", progActionNames().value(progActionByApp(app)) },
        { "applyParent", app.applyParent },
        { "applyChild", app.applyChild },
        { "applySpecChild", app.applySpecChild },
        { "killChild", app.killChild },
        { "blockInbound", app.blockInbound },
        { "blockOutbound", app.blockOutbound },
        { "lanOnly", app.lanOnly },
        { "parked", app.parked },
        { "logStat", app.logStat },
        { "logAllowedConn", app.logAllowedConn },
        { "logBlockedConn", app.logBlockedConn },
        { "inLimitEnabled", app.inLimitEnabled },
        { "outLimitEnabled", app.outLimitEnabled },
        { "inLimit", QVariant::fromValue(app.speedLimits.in_limit_id) },
        { "outLimit", QVariant::fromValue(app.speedLimits.out_limit_id) },
        { "rule", confRuleManager()->ruleNameById(app.ruleId) },
        { "groups", ControlCommandUtil::maskIds(app.groups) },
        { "acceptZones", ControlCommandUtil::maskIds(app.zones.accept_mask) },
        { "rejectZones", ControlCommandUtil::maskIds(app.zones.reject_mask) },
    };
}

void fillAppFlags(App &app, const QVariantMap &fields)
{
    app.applyParent = fields["applyParent"].toBool();
    app.applyChild = fields["applyChild"].toBool();
    app.applySpecChild = fields["applySpecChild"].toBool();
    app.killChild = fields["killChild"].toBool();
    app.blockInbound = fields["blockInbound"].toBool();
    app.blockOutbound = fields["blockOutbound"].toBool();
    app.lanOnly = fields["lanOnly"].toBool();
    app.parked = fields["parked"].toBool();
    app.logStat = fields["logStat"].toBool();
    app.logAllowedConn = fields["logAllowedConn"].toBool();
    app.logBlockedConn = fields["logBlockedConn"].toBool();
    app.inLimitEnabled = fields["inLimitEnabled"].toBool();
    app.outLimitEnabled = fields["outLimitEnabled"].toBool();
}

void fillAppIds(App &app, const QVariantMap &fields)
{
    app.speedLimits.in_limit_id = fields["inLimit"].toUInt();
    app.speedLimits.out_limit_id = fields["outLimit"].toUInt();
    app.groups = ControlCommandUtil::idsMask(fields["groups"]);
    app.zones.accept_mask = ControlCommandUtil::idsMask(fields["acceptZones"]);
    app.zones.reject_mask = ControlCommandUtil::idsMask(fields["rejectZones"]);
}

bool fillAppAction(ProcessCommandResult &r, App &app, const QVariantMap &fields)
{
    // The App's actions only
    const QStringList actionNames = progActionNames().mid(0, int(std::size(appActions)));

    const int progAction =
            ControlCommandUtil::nameIndex(fields, "action", actionNames, r.errorMessage);
    if (progAction < 0)
        return false;

    app.setAction(appActionByProg(ProgAction(progAction)));

    return true;
}

bool setAppFields(ProcessCommandResult &r, App &app, const QVariantList &args)
{
    QVariantMap fields = appFields(app);

    if (!ControlCommandUtil::parseFieldArgs(args, 2, fields, r.errorMessage))
        return false;

    if (!(fillAppAction(r, app, fields)
                && ControlCommandUtil::findRuleIdByName(
                        fields["rule"].toString(), app.ruleId, r.errorMessage)))
        return false;

    fillAppFlags(app, fields);
    fillAppIds(app, fields);

    return confAppManager()->updateApp(app);
}

bool getAppFields(ProcessCommandResult &r, const App &app)
{
    r.errorMessage = ControlCommandUtil::fieldsToTextWithId(appFields(app), app.appId);

    return true;
}

bool setAppFilters(ProcessCommandResult &r, App &app, const QString &filtersText)
{
    Rule rule;
    ConfUtil::parseAppFiltersText(filtersText, rule);

    if (!ControlCommandUtil::validateRuleText(r, rule.ruleText))
        return false;

    app.filtersText = ConfUtil::appFiltersText(rule);

    return confAppManager()->updateApp(app);
}

bool getAppFilters(ProcessCommandResult &r, const App &app)
{
    // The client writes the text to the file
    r.errorMessage = app.filtersText;

    return true;
}

bool processCommandAppAction(
        ProcessCommandResult &r, const QVariantList &args, ProgAction progAction, bool report)
{
    App app;

    if (!loadAppByPath(r, app, argAppPath(args)))
        return true;

    if (report) {
        return reportCommandProgAction(r, app);
    }

    switch (progAction) {
    case ProgActionDel: {
        return confAppManager()->deleteApps({ app.appId });
    }
    case ProgActionGet: {
        return getAppFields(r, app);
    }
    case ProgActionSet: {
        return setAppFields(r, app, args);
    }
    case ProgActionSetFilters: {
        return setAppFilters(r, app, args.value(2).toString());
    }
    default: // Get Filters
        return getAppFilters(r, app);
    }
}

bool processCommandProgAction(
        ProcessCommandResult &r, const QVariantList &args, ProgAction progAction, bool report)
{
    // Report, Delete or the App's fields
    if (report || progAction >= ProgActionDel) {
        return processCommandAppAction(r, args, progAction, report);
    }

    const QString appPath = argAppPath(args);

    switch (progAction) {
    case ProgActionAdd: {
        return windowManager()->showProgramEditForm(appPath);
    }
    default: // Allow, Block, Kill or Ask
        return confAppManager()->addOrUpdateAppPath(appPath, appActionByProg(progAction));
    }
}

}

bool ControlCommandProg::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const
{
    bool report = false;
    const ProgAction progAction = progActionByText(p.args.value(0).toString(), report);

    const bool isValidAction = (progAction != ProgActionInvalid || report);
    if (!isValidAction || p.args.size() < progActionArgsCount(progAction)) {
        r.errorMessage = "Usage: prog allow|block|kill|ask|add|del|report|get <app-path>"
                         " | prog set <app-path> <key>=<value>..."
                         " | prog set-filters|get-filters <app-path> <file-path>";
        return false;
    }

    if (progAction != ProgActionAdd && argAppPath(p.args).isEmpty()) {
        r.errorMessage = "<app-path> is empty";
        return false;
    }

    if (!ControlCommandUtil::checkCommandActionPassword(r, progAction))
        return false;

    return processCommandProgAction(r, p.args, progAction, report);
}

bool ControlCommandProg::needsWindows(const QVariantList &args) const
{
    // The add action shows the program's edit form
    bool report = false;
    return progActionByText(args.value(0).toString(), report) == ProgActionAdd;
}

ControlCommandBase::ClientFileArg ControlCommandProg::clientFileArg(const QVariantList &args) const
{
    switch (clientProgAction(args)) {
    case ProgActionSetFilters:
        return ClientFileRead;
    case ProgActionGetFilters:
        return ClientFileWrite;
    default:
        return ClientFileNone;
    }
}
