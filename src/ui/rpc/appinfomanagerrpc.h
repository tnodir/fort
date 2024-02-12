#ifndef APPINFOMANAGERRPC_H
#define APPINFOMANAGERRPC_H

#include <appinfo/appinfomanager.h>

class RpcManager;

class AppInfoManagerRpc : public AppInfoManager
{
    Q_OBJECT

public:
    explicit AppInfoManagerRpc(const QString &filePath, QObject *parent = nullptr);

    static void setupServerSignals(RpcManager *rpcManager);

public slots:
    void lookupAppInfo(const QString &appPath) override;

protected:
    void updateAppAccessTime(const QString & /*appPath*/) override { }
};

#endif // APPINFOMANAGERRPC_H
