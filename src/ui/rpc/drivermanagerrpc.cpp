#include "drivermanagerrpc.h"

#include <control/controlworker.h>
#include <fortglobal.h>
#include <rpc/rpcmanager.h>

using namespace Fort;

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
    auto driverManager = Fort::driverManager();

    return { driverManager->errorCode(), driverManager->isDeviceOpened() };
}

bool DriverManagerRpc::processInitClient(ControlWorker *w)
{
    return w->sendCommand(Control::Rpc_DriverManager_updateState, updateState_args());
}

bool DriverManagerRpc::processServerCommand(
        const ProcessCommandArgs &p, ProcessCommandResult & /*r*/)
{
    auto driverManager = Fort::driverManager();

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
    auto driverManager = Fort::driverManager();

    const auto updateClientStates = [=] {
        rpcManager->invokeOnClients(
                Control::Rpc_DriverManager_updateState, DriverManagerRpc::updateState_args());
    };

    connect(driverManager, &DriverManager::errorCodeChanged, rpcManager, updateClientStates);
    connect(driverManager, &DriverManager::isDeviceOpenedChanged, rpcManager, updateClientStates);
}
