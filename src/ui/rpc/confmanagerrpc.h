#ifndef CONFMANAGERRPC_H
#define CONFMANAGERRPC_H

#include <conf/confmanager.h>

class RpcManager;
class TaskManager;

class ConfManagerRpc : public ConfManager
{
    Q_OBJECT

public:
    explicit ConfManagerRpc(const QString &filePath, QObject *parent = nullptr);

    bool addApp(const App &app) override;
    void deleteApps(const QVector<qint64> &appIdList) override;
    bool purgeApps() override;
    bool updateApp(const App &app) override;
    void updateAppsBlocked(
            const QVector<qint64> &appIdList, bool blocked, bool killProcess) override;
    bool updateAppName(qint64 appId, const QString &appName) override;

    bool addZone(Zone &zone) override;
    bool deleteZone(int zoneId) override;
    bool updateZone(const Zone &zone) override;
    bool updateZoneName(int zoneId, const QString &zoneName) override;
    bool updateZoneEnabled(int zoneId, bool enabled) override;

    bool checkPassword(const QString &password) override;

    bool updateDriverConf(bool /*onlyFlags*/ = false) override { return false; }

    void onConfChanged(const QVariant &confVar);

    static QVariantList appToVarList(const App &app);
    static App varListToApp(const QVariantList &v);

protected:
    void purgeAppsOnStart() override { }

    void setupAppEndTimer() override { }

    bool saveConf(FirewallConf &newConf) override;

private:
    bool saving() const { return m_saving; }
    void setSaving(bool v) { m_saving = v; }

private:
    bool m_saving = false;
};

#endif // CONFMANAGERRPC_H
