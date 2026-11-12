#include "controlcommandbase.h"

#include <control/controlworker.h>

#include "controlcommandutil.h"

bool ControlCommandBase::prepareClientArgs(QVariantList &args, ProcessCommandResult &r) const
{
    if (clientFileArg(args) != ClientFileRead)
        return true;

    return ControlCommandUtil::readClientFileArg(args, r);
}

bool ControlCommandBase::processClientResult(
        const QVariantList &args, ProcessCommandResult &r) const
{
    if (clientFileArg(args) != ClientFileWrite)
        return true;

    return ControlCommandUtil::writeClientFileArg(args, r);
}

int ControlCommandBase::resultWaitMsecs(const QVariantList & /*args*/) const
{
    return ControlWorker::defaultWaitMsecs;
}
