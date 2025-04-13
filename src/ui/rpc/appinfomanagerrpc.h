#ifndef APPINFOMANAGERRPC_H
#define APPINFOMANAGERRPC_H

#include <appinfo/appinfomanager.h>
#include <control/control_types.h>

class RpcManager;

class AppInfoManagerRpc : public AppInfoManager
{
    Q_OBJECT

public:
    explicit AppInfoManagerRpc(const QString &filePath, bool noCache, QObject *parent = nullptr);

    static bool processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);

    static void setupServerSignals(RpcManager *rpcManager);

public slots:
    void lookupAppInfo(const QString &appPath) override;
};

#endif // APPINFOMANAGERRPC_H
