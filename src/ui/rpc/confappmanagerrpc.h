#ifndef CONFAPPMANAGERRPC_H
#define CONFAPPMANAGERRPC_H

#include <conf/confappmanager.h>

class RpcManager;

class ConfAppManagerRpc : public ConfAppManager
{
    Q_OBJECT

public:
    explicit ConfAppManagerRpc(QObject *parent = nullptr);

    bool addOrUpdateApp(App &app, bool onlyUpdate = false) override;
    bool updateApp(App &app) override;
    bool updateAppName(qint64 appId, const QString &appName) override;
    bool deleteApps(const QVector<qint64> &appIdList) override;
    bool purgeApps() override;
    bool updateAppsBlocked(
            const QVector<qint64> &appIdList, bool blocked, bool killProcess) override;

    bool updateDriverConf(bool /*onlyFlags*/ = false) override { return false; }

    static QVariantList appToVarList(const App &app);
    static App varListToApp(const QVariantList &v);

protected:
    void setupDriveListManager() override { }

    void setupAppEndTimer() override { }
};

#endif // CONFAPPMANAGERRPC_H
