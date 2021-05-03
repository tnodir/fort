#include "confmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include "../fortmanager.h"
#include "../rpc/rpcmanager.h"

ConfManagerRpc::ConfManagerRpc(const QString &filePath, FortManager *fortManager, QObject *parent) :
    ConfManager(filePath, fortManager, parent, SqliteDb::OpenDefaultReadOnly)
{
}

RpcManager *ConfManagerRpc::rpcManager() const
{
    return fortManager()->rpcManager();
}

void ConfManagerRpc::onConfSaved(bool onlyFlags, int confVersion)
{
    if (this->confVersion() == confVersion)
        return;

    // TODO: reload conf

    setConfVersion(confVersion);

    emit confSaved(onlyFlags);
}

void ConfManagerRpc::setupAppEndTimer() { }
