#include "drivermanagerrpc.h"

#include <control/controlworker.h>
#include <rpc/rpcmanager.h>
#include <util/ioc/ioccontainer.h>

DriverManagerRpc::DriverManagerRpc(QObject *parent) : DriverManager(parent, /*useDevice=*/false) { }

void DriverManagerRpc::setIsDeviceOpened(bool v)
{
    if (m_isDeviceOpened != v) {
        m_isDeviceOpened = v;
        emit isDeviceOpenedChanged();
    }
}

void DriverManagerRpc::updateState(quint32 errorCode, bool isDeviceOpened)
{
    setIsDeviceOpened(isDeviceOpened);
    setErrorCode(errorCode);
}

bool DriverManagerRpc::openDevice()
{
    return false;
}

bool DriverManagerRpc::closeDevice()
{
    updateState(/*errorCode=*/0, /*isDeviceOpened=*/false);

    return false;
}

QVariantList DriverManagerRpc::updateState_args()
{
    auto driverManager = IoC<DriverManager>();

    return { driverManager->errorCode(), driverManager->isDeviceOpened() };
}

bool DriverManagerRpc::processInitClient(ControlWorker *w)
{
    return w->sendCommand(Control::Rpc_DriverManager_updateState, updateState_args());
}

bool DriverManagerRpc::processServerCommand(const ProcessCommandArgs &p, QVariantList & /*resArgs*/,
        bool & /*ok*/, bool & /*isSendResult*/)
{
    auto driverManager = IoC<DriverManager>();

    switch (p.command) {
    case Control::Rpc_DriverManager_updateState: {
        if (auto dm = qobject_cast<DriverManagerRpc *>(driverManager)) {
            dm->updateState(p.args.value(0).toUInt(), p.args.value(1).toBool());
        }
        return true;
    }
    default:
        return false;
    }
}

void DriverManagerRpc::setupServerSignals(RpcManager *rpcManager)
{
    auto driverManager = IoC<DriverManager>();

    const auto updateClientStates = [=] {
        rpcManager->invokeOnClients(
                Control::Rpc_DriverManager_updateState, DriverManagerRpc::updateState_args());
    };

    connect(driverManager, &DriverManager::errorCodeChanged, rpcManager, updateClientStates);
    connect(driverManager, &DriverManager::isDeviceOpenedChanged, rpcManager, updateClientStates);
}
