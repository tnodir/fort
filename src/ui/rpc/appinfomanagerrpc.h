#ifndef APPINFOMANAGERRPC_H
#define APPINFOMANAGERRPC_H

#include "../appinfo/appinfomanager.h"

class FortManager;
class RpcManager;

class AppInfoManagerRpc : public AppInfoManager
{
    Q_OBJECT

public:
    explicit AppInfoManagerRpc(
            const QString &filePath, FortManager *fortManager, QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }
    RpcManager *rpcManager() const;

public slots:
    void lookupAppInfo(const QString &appPath) override;

protected:
    void updateAppAccessTime(const QString & /*appPath*/) override { }

private:
    FortManager *m_fortManager = nullptr;
};

#endif // APPINFOMANAGERRPC_H
