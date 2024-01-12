#ifndef CONFAPPMANAGERRPC_H
#define CONFAPPMANAGERRPC_H

#include <conf/confappmanager.h>

class RpcManager;

class ConfAppManagerRpc : public ConfAppManager
{
    Q_OBJECT

public:
    explicit ConfAppManagerRpc(QObject *parent = nullptr);

    bool addApp(const App &app) override;
    bool deleteApps(const QVector<qint64> &appIdList) override;
    bool purgeApps() override;
    bool updateApp(const App &app) override;
    bool updateAppsBlocked(
            const QVector<qint64> &appIdList, bool blocked, bool killProcess) override;
    bool updateAppName(qint64 appId, const QString &appName) override;

    bool updateDriverConf(bool /*onlyFlags*/ = false) override { return false; }

    static QVariantList appToVarList(const App &app);
    static App varListToApp(const QVariantList &v);

protected:
    void setupDriveListManager() override { }

    void purgeAppsOnStart() override { }

    void setupAppEndTimer() override { }
};

#endif // CONFAPPMANAGERRPC_H
