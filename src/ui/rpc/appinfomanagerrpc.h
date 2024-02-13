#ifndef APPINFOMANAGERRPC_H
#define APPINFOMANAGERRPC_H

#include <appinfo/appinfomanager.h>

class RpcManager;

struct ProcessCommandArgs;

class AppInfoManagerRpc : public AppInfoManager
{
    Q_OBJECT

public:
    explicit AppInfoManagerRpc(const QString &filePath, QObject *parent = nullptr);

    static bool processServerCommand(
            const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult);

    static void setupServerSignals(RpcManager *rpcManager);

public slots:
    void lookupAppInfo(const QString &appPath) override;

protected:
    void updateAppAccessTime(const QString & /*appPath*/) override { }
};

#endif // APPINFOMANAGERRPC_H
