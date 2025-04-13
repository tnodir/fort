#ifndef CONFMANAGERRPC_H
#define CONFMANAGERRPC_H

#include <conf/confmanager.h>
#include <control/control_types.h>

class RpcManager;
class TaskManager;

class ConfManagerRpc : public ConfManager
{
    Q_OBJECT

public:
    explicit ConfManagerRpc(const QString &filePath, QObject *parent = nullptr);

    bool saveConf(FirewallConf &newConf) override;

    bool exportMasterBackup(const QString &path) override;
    bool importMasterBackup(const QString &path) override;

    bool checkPassword(const QString &password) override;

    void onConfChanged(const QVariant &confVar);

    static bool processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);

    static void setupServerSignals(RpcManager *rpcManager);

protected:
    void setupTimers() override { }

    bool applyConfPeriods(bool /*onlyFlags*/) override { return false; }
    void applyAutoLearnSeconds() override { }

private:
    bool saving() const { return m_saving; }
    void setSaving(bool v) { m_saving = v; }

private:
    bool m_saving = false;
};

#endif // CONFMANAGERRPC_H
