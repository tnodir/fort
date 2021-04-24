#include "rpcmanager.h"

#include "../fortmanager.h"
#include "../fortsettings.h"

RpcManager::RpcManager(FortManager *fortManager, QObject *parent) :
    QObject(parent), m_fortManager(fortManager)
{
}

FortSettings *RpcManager::settings() const
{
    return fortManager()->settings();
}

ControlManager *RpcManager::controlManager() const
{
    return fortManager()->controlManager();
}

void RpcManager::initialize()
{
    if (settings()->isService()) {
        // TODO: Initialize RPC Server
    } else {
        // TODO: Initialize RPC Client
    }
}
