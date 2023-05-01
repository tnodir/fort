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
    bool deleteApp(qint64 appId) override;
    bool purgeApps() override;
    bool updateApp(const App &app) override;
    bool updateAppBlocked(qint64 appId, bool blocked, bool killProcess = false) override;
    bool updateAppName(qint64 appId, const QString &appName) override;

    bool addZone(Zone &zone) override;
    bool deleteZone(int zoneId) override;
    bool updateZone(const Zone &zone) override;
    bool updateZoneName(int zoneId, const QString &zoneName) override;
    bool updateZoneEnabled(int zoneId, bool enabled) override;

    bool checkPassword(const QString &password) override;

    bool updateDriverConf(bool /*onlyFlags*/ = false) override { return false; }

    void onConfChanged(const QVariant &confVar);

protected:
    void setupAppEndTimer() override { }

    bool saveConf(FirewallConf &newConf) override;

private:
    bool saving() const { return m_saving; }
    void setSaving(bool v) { m_saving = v; }

private:
    bool m_saving = false;
};

#endif // CONFMANAGERRPC_H
