#include "controlcommandprog.h"

#include <conf/confappmanager.h>
#include <manager/windowmanager.h>
#include <util/ioc/ioccontainer.h>

namespace {

enum ProgAction : quint32 {
    ProgActionNone = 0,
    ProgActionAdd = (1 << 0),
    ProgActionDel = (1 << 1),
    ProgActionAllow = (1 << 2),
    ProgActionBlock = (1 << 3),
    ProgActionKill = (1 << 4),
};

ProgAction progActionByText(const QString &commandText)
{
    if (commandText == "add")
        return ProgActionAdd;

    if (commandText == "del")
        return ProgActionDel;

    if (commandText == "allow")
        return ProgActionAllow;

    if (commandText == "block")
        return ProgActionBlock;

    if (commandText == "kill")
        return ProgActionKill;

    return ProgActionNone;
}

bool processCommandProgAction(ProgAction progAction, const QString &appPath)
{
    switch (progAction) {
    case ProgActionAdd: {
        return IoC<WindowManager>()->showProgramEditForm(appPath);
    }
    case ProgActionDel: {
        return IoC<ConfAppManager>()->deleteAppPath(appPath);
    }
    case ProgActionAllow:
    case ProgActionBlock:
    case ProgActionKill: {
        const bool blocked = (progAction != ProgActionAllow);
        const bool killProcess = (progAction == ProgActionKill);

        return IoC<ConfAppManager>()->addOrUpdateAppPath(appPath, blocked, killProcess);
    }
    default:
        return false;
    }
}

}

bool ControlCommandProg::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    const ProgAction progAction = progActionByText(p.args.value(0).toString());
    if (progAction == ProgActionNone) {
        r.errorMessage = "Usage: prog add|del|allow|block|kill|show <app-path>";
        return false;
    }

    if (!checkCommandActionPassword(r, progAction, ProgActionAdd))
        return false;

    const QString appPath = p.args.value(1).toString();

    return processCommandProgAction(progAction, appPath);
}
