#include "controlcommandconf.h"

#include <conf/confappmanager.h>
#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <fortglobal.h>

#include "controlcommandutil.h"

using namespace Fort;

namespace {

enum ConfAction : quint32 {
    ConfActionNone = 0,
    ConfActionUpdateDriver = (1 << 0),
    ConfActionSet = (1 << 1),
    ConfActionGet = (1 << 2),
};

ConfAction confActionByText(const QString &commandText)
{
    if (commandText == "update-driver")
        return ConfActionUpdateDriver;

    if (commandText == "set")
        return ConfActionSet;

    if (commandText == "get")
        return ConfActionGet;

    return ConfActionNone;
}

bool setConfFlags(ProcessCommandResult &r, const QVariantList &args)
{
    auto confManager = Fort::confManager();

    auto &conf = confManager->conf();

    QVariantMap flags = conf.flagsToVariant().toMap();

    if (!ControlCommandUtil::parseFieldArgs(args, 1, flags, r.errorMessage))
        return false;

    conf.flagsFromVariant(flags);

    return confManager->saveFlags();
}

bool getConfFlags(ProcessCommandResult &r)
{
    const auto &conf = confManager()->conf();

    r.errorMessage = ControlCommandUtil::fieldsToText(conf.flagsToVariant().toMap());

    return true;
}

bool processCommandConfAction(
        ProcessCommandResult &r, const QVariantList &args, ConfAction confAction)
{
    switch (confAction) {
    case ConfActionUpdateDriver: {
        return confAppManager()->updateDriverConf();
    }
    case ConfActionSet: {
        return setConfFlags(r, args);
    }
    case ConfActionGet: {
        return getConfFlags(r);
    }
    default:
        return false;
    }
}

}

bool ControlCommandConf::processCommand(const ProcessCommandArgs &p, ProcessCommandResult &r) const
{
    const ConfAction confAction = confActionByText(p.args.value(0).toString());

    const bool isSetEmpty = (confAction == ConfActionSet && p.args.size() < 2);
    if (confAction == ConfActionNone || isSetEmpty) {
        r.errorMessage = "Usage: conf update-driver | conf set <key>=<value>... | conf get";
        return false;
    }

    if (!ControlCommandUtil::checkCommandActionPassword(r, confAction, ConfActionUpdateDriver))
        return false;

    return processCommandConfAction(r, p.args, confAction);
}
