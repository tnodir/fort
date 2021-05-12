#ifndef STATMANAGERRPC_H
#define STATMANAGERRPC_H

#include "../stat/statmanager.h"

class FortManager;
class RpcManager;

class StatManagerRpc : public StatManager
{
    Q_OBJECT

public:
    explicit StatManagerRpc(
            const QString &filePath, FortManager *fortManager, QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }
    RpcManager *rpcManager() const;

    void setConf(const FirewallConf * /*conf*/) override { }

public slots:
    bool clear() override;

private:
    FortManager *m_fortManager = nullptr;
};

#endif // STATMANAGERRPC_H
