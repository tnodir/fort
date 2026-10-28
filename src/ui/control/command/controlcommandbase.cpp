#include "controlcommandbase.h"

bool ControlCommandBase::prepareClientArgs(
        QVariantList & /*args*/, ProcessCommandResult & /*r*/) const
{
    return true;
}

bool ControlCommandBase::processClientResult(
        const QVariantList & /*args*/, ProcessCommandResult & /*r*/) const
{
    return true;
}
