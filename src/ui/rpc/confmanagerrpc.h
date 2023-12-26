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

    bool exportBackup(const QString &path) override;
    bool importBackup(const QString &path) override;

    bool addZone(Zone &zone) override;
    bool deleteZone(int zoneId) override;
    bool updateZone(const Zone &zone) override;
    bool updateZoneName(int zoneId, const QString &zoneName) override;
    bool updateZoneEnabled(int zoneId, bool enabled) override;

    bool checkPassword(const QString &password) override;

    void onConfChanged(const QVariant &confVar);

protected:
    bool saveConf(FirewallConf &newConf) override;

private:
    bool saving() const { return m_saving; }
    void setSaving(bool v) { m_saving = v; }

private:
    bool m_saving = false;
};

#endif // CONFMANAGERRPC_H
