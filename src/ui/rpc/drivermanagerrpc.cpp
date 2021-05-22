#include "drivermanagerrpc.h"

#include "../control/controlworker.h"
#include "../fortmanager.h"
#include "../rpc/rpcmanager.h"

DriverManagerRpc::DriverManagerRpc(FortManager *fortManager, QObject *parent) :
    DriverManager(parent, false), m_isDeviceOpened(false), m_fortManager(fortManager)
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
    rpcManager()->client()->abort();

    updateState(0, false);

    return false;
}
