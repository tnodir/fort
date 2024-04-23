#ifndef AUTOUPDATEMANAGERRPC_H
#define AUTOUPDATEMANAGERRPC_H

#include <manager/autoupdatemanager.h>

class RpcManager;

class AutoUpdateManagerRpc : public AutoUpdateManager
{
    Q_OBJECT

public:
    explicit AutoUpdateManagerRpc(const QString &cachePath, QObject *parent = nullptr);
};

#endif // AUTOUPDATEMANAGERRPC_H
