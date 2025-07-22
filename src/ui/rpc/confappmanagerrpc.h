#ifndef CONFAPPMANAGERRPC_H
#define CONFAPPMANAGERRPC_H

#include <conf/confappmanager.h>
#include <control/control_types.h>

class RpcManager;

class ConfAppManagerRpc : public ConfAppManager
{
    Q_OBJECT

public:
    explicit ConfAppManagerRpc(QObject *parent = nullptr);

    bool addOrUpdateAppPath(const QString &appOriginPath, bool blocked, bool killProcess) override;
    bool deleteAppPath(const QString &appOriginPath) override;

    bool addOrUpdateApp(App &app, bool onlyUpdate = false) override;
    bool updateApp(App &app) override;
    bool updateAppName(qint64 appId, const QString &appName) override;
    bool deleteApps(const QVector<qint64> &appIdList) override;
    bool clearAlerts() override;
    bool purgeApps() override;
    bool updateAppsBlocked(
            const QVector<qint64> &appIdList, bool blocked, bool killProcess) override;
    bool updateAppsTimer(const QVector<qint64> &appIdList, int minutes) override;

    bool importAppsBackup(const QString &path) override;

    bool canUpdateDriverConf() const override { return false; }
    bool updateDriverConf(bool onlyFlags = false) override;

    static QVariantList appToVarList(const App &app);
    static App varListToApp(const QVariantList &v);

    static bool processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);

    static void setupServerSignals(RpcManager *rpcManager);

protected:
    void setupConfManager() override { }

    void setupAppEndTimer() override { }
};

#endif // CONFAPPMANAGERRPC_H
