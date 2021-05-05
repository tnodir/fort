#ifndef CONFMANAGERRPC_H
#define CONFMANAGERRPC_H

#include "../conf/confmanager.h"

class RpcManager;

class ConfManagerRpc : public ConfManager
{
    Q_OBJECT

public:
    explicit ConfManagerRpc(
            const QString &filePath, FortManager *fortManager, QObject *parent = nullptr);

    RpcManager *rpcManager() const;

    void onConfChanged(const QVariant &confVar);

protected:
    void setupAppEndTimer() override { }

    bool saveConf(FirewallConf &newConf) override;

    bool saving() const { return m_saving; }
    void setSaving(bool v) { m_saving = v; }

private:
    bool m_saving = false;
};

#endif // CONFMANAGERRPC_H
