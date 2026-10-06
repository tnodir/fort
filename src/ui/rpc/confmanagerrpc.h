#ifndef CONFMANAGERRPC_H
#define CONFMANAGERRPC_H

#include <conf/confmanager.h>
#include <control/control_types.h>

class IniOptions;
class RpcManager;
class TaskManager;

struct FilterSimConn;

class ConfManagerRpc : public ConfManager
{
    Q_OBJECT

public:
    explicit ConfManagerRpc(const QString &filePath, QObject *parent = nullptr);

    bool saveConf(FirewallConf &conf, IniOptions &ini) override;

    bool saveVariant(const QVariant &confVar) override;

    bool exportMasterBackup(const QString &path) override;
    bool importMasterBackup(const QString &path) override;

    bool checkPassword(const QString &password) override;

    bool simulateConn(FilterSimConn &simConn) override;

    void onConfChanged(const QVariant &confVar);

    static QVariantList simConnToVarList(const FilterSimConn &simConn);
    static FilterSimConn varListToSimConn(const QVariantList &v);

    static bool processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);

    static void setupServerSignals(RpcManager *rpcManager);

protected:
    void setupTimers() override { }

    void applyAutoLearnSeconds() override { }

private:
    bool saving() const { return m_saving; }
    void setSaving(bool v) { m_saving = v; }

private:
    bool m_saving = false;
};

#endif // CONFMANAGERRPC_H
