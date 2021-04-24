#include "drivermanagerrpc.h"

#include "../fortmanager.h"
#include "../rpc/rpcmanager.h"

DriverManagerRpc::DriverManagerRpc(FortManager *fortManager, QObject *parent) :
    DriverManager(parent), m_isDeviceOpened(false), m_fortManager(fortManager)
{
}

bool DriverManagerRpc::isDeviceOpened() const
{
    return m_isDeviceOpened;
}

RpcManager *DriverManagerRpc::rpcManager() const
{
    return fortManager()->rpcManager();
}

void DriverManagerRpc::reinstallDriver() { }

void DriverManagerRpc::uninstallDriver() { }

bool DriverManagerRpc::openDevice()
{
    return false;
}

bool DriverManagerRpc::closeDevice()
{
    return false;
}
