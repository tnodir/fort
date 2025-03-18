#include "drivelistmanagerrpc.h"

#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

DriveListManagerRpc::DriveListManagerRpc(QObject *parent) : DriveListManager(parent) { }

bool DriveListManagerRpc::processServerCommand(const ProcessCommandArgs &p,
        QVariantList & /*resArgs*/, bool & /*ok*/, bool & /*isSendResult*/)
{
    auto driveListManager = IoC<DriveListManager>();

    switch (p.command) {
    case Control::Rpc_DriveListManager_onDriveListChanged: {
        driveListManager->onDriveListChanged();
        return true;
    }
    default:
        return false;
    }
}

void DriveListManagerRpc::onDriveListChanged()
{
    IoC<RpcManager>()->invokeOnServer(Control::Rpc_DriveListManager_onDriveListChanged);
}
