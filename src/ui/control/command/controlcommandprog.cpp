#include "controlcommandprog.h"

#include <algorithm>

#include <conf/app.h>
#include <conf/confappmanager.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>

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
};

QStringList progActionNames()
{
    // Sync with enum ProgAction
    return { "allow", "block", "kill", "ask", "add", "del" };
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

bool reportCommandProgAction(ProcessCommandResult &r, const QString &appPath)
{
    const App app = confAppManager()->appByPath(appPath);

    if (!app.isValid()) {
        r.commandResult = Control::CommandResultError;
        r.errorMessage = "App not found";
        return true;
    }

    const auto progAction = progActionByApp(app);

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
    case ProgActionAdd: {
        return windowManager()->showProgramEditForm(appPath);
    }
    case ProgActionDel: {
        return confAppManager()->deleteAppPath(appPath);
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

    if (progAction == ProgActionInvalid && !report) {
        r.errorMessage = "Usage: prog allow|block|kill|ask|add|del|report <app-path>";
        return false;
    }

    const QString appPath = p.args.value(1).toString().trimmed();

    if (progAction != ProgActionAdd && appPath.isEmpty()) {
        r.errorMessage = "<app-path> is empty";
        return false;
    }

    if (!ControlCommandUtil::checkCommandActionPassword(r, progAction))
        return false;

    return processCommandProgAction(r, appPath, progAction, report);
}
