#include "controlcommandprog.h"

#include <conf/app.h>
#include <conf/confappmanager.h>
#include <manager/windowmanager.h>
#include <util/ioc/ioccontainer.h>

namespace {

enum ProgAction : qint8 {
    ProgActionInvalid = -1,
    ProgActionAllow = 0,
    ProgActionBlock,
    ProgActionKill,
    ProgActionAdd,
    ProgActionDel,
};

QStringList progActionNames()
{
    // Sync with enum ProgAction
    return { "allow", "block", "kill" };
}

ProgAction progActionByText(const QString &commandText, bool &report)
{
    if (commandText == "allow")
        return ProgActionAllow;

    if (commandText == "block")
        return ProgActionBlock;

    if (commandText == "kill")
        return ProgActionKill;

    if (commandText == "add")
        return ProgActionAdd;

    if (commandText == "del")
        return ProgActionDel;

    if (commandText == "report") {
        report = true;
    }

    return ProgActionInvalid;
}

bool reportCommandProgAction(ProcessCommandResult &r, const QString &appPath)
{
    const App app = IoC<ConfAppManager>()->appByPath(appPath);

    if (!app.isValid()) {
        r.commandResult = Control::CommandResultError;
        r.errorMessage = "App not found";
        return true;
    }

    const auto progAction =
            app.killProcess ? ProgActionKill : (app.blocked ? ProgActionBlock : ProgActionAllow);

    r.commandResult = Control::CommandResult(Control::CommandResultBase + progAction);

    r.errorMessage = progActionNames().value(progAction);

    return true;
}

bool processCommandProgAction(
        ProcessCommandResult &r, const QString &appPath, ProgAction progAction, bool report)
{
    if (report) {
        return reportCommandProgAction(r, appPath);
    }

    switch (progAction) {
    case ProgActionAllow:
    case ProgActionBlock:
    case ProgActionKill: {
        const bool blocked = (progAction != ProgActionAllow);
        const bool killProcess = (progAction == ProgActionKill);

        return IoC<ConfAppManager>()->addOrUpdateAppPath(appPath, blocked, killProcess);
    }
    case ProgActionAdd: {
        return IoC<WindowManager>()->showProgramEditForm(appPath);
    }
    case ProgActionDel: {
        return IoC<ConfAppManager>()->deleteAppPath(appPath);
    }
    default:
        return false;
    }
}

}

bool ControlCommandProg::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    bool report = false;
    const ProgAction progAction = progActionByText(p.args.value(0).toString(), report);

    if (progAction == ProgActionInvalid && !report) {
        r.errorMessage = "Usage: prog allow|block|kill|add|del|report <app-path>";
        return false;
    }

    if (!checkCommandActionPassword(r, progAction))
        return false;

    const QString appPath = p.args.value(1).toString();

    const bool ok = processCommandProgAction(r, appPath, progAction, report);

    uncheckCommandActionPassword();

    return ok;
}
