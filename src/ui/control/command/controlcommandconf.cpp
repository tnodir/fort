#include "controlcommandconf.h"

#include <conf/confappmanager.h>
#include <util/ioc/ioccontainer.h>

namespace {

enum ConfAction : quint32 {
    ConfActionNone = 0,
    ConfActionUpdateDriver = (1 << 0),
};

ConfAction confActionByText(const QString &commandText)
{
    if (commandText == "update-driver")
        return ConfActionUpdateDriver;

    return ConfActionNone;
}

bool processCommandConfAction(ConfAction confAction)
{
    switch (confAction) {
    case ConfActionUpdateDriver: {
        return IoC<ConfAppManager>()->updateDriverConf();
    }
    default:
        return false;
    }
}

}

bool ControlCommandConf::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r)
{
    const ConfAction confAction = confActionByText(p.args.value(0).toString());
    if (confAction == ConfActionNone) {
        r.errorMessage = "Usage: conf update-driver";
        return false;
    }

    if (!checkCommandActionPassword(r, confAction, ConfActionUpdateDriver))
        return false;

    return processCommandConfAction(confAction);
}
