#include "drivermanagerrpc.h"

#include "../fortmanager.h"
#include "../rpc/rpcmanager.h"

DriverManagerRpc::DriverManagerRpc(FortManager *fortManager, QObject *parent) :
    DriverManager(parent), m_isDeviceOpened(false), m_fortManager(fortManager)
{
}

void DriverManagerRpc::setIsDeviceOpened(bool v)
{
    if (m_isDeviceOpened != v) {
        m_isDeviceOpened = v;
        emit isDeviceOpenedChanged();
    }
}

RpcManager *DriverManagerRpc::rpcManager() const
{
    return fortManager()->rpcManager();
}

bool DriverManagerRpc::reinstallDriver()
{
    return rpcManager()->doOnServer(Control::Rpc_DriverManager_reinstallDriver);
}

bool DriverManagerRpc::uninstallDriver()
{
    return rpcManager()->doOnServer(Control::Rpc_DriverManager_uninstallDriver);
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
    return false;
}
