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

    bool addApp(const QString &appPath, const QString &appName, const QDateTime &endTime,
            int groupIndex, bool useGroupPerm, bool applyChild, bool blocked) override;
    bool deleteApp(qint64 appId) override;
    bool purgeApps() override;
    bool updateApp(qint64 appId, const QString &appPath, const QString &appName,
            const QDateTime &endTime, int groupIndex, bool useGroupPerm, bool applyChild,
            bool blocked) override;
    bool updateAppBlocked(qint64 appId, bool blocked) override;
    bool updateAppName(qint64 appId, const QString &appName) override;

    bool addZone(const QString &zoneName, const QString &sourceCode, const QString &url,
            const QString &formData, bool enabled, bool customUrl, int &zoneId) override;
    bool deleteZone(int zoneId) override;
    bool updateZone(int zoneId, const QString &zoneName, const QString &sourceCode,
            const QString &url, const QString &formData, bool enabled, bool customUrl) override;
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
