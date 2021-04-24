#ifndef QUOTAMANAGERRPC_H
#define QUOTAMANAGERRPC_H

#include "../stat/quotamanager.h"

class FortManager;
class RpcManager;

class QuotaManagerRpc : public QuotaManager
{
    Q_OBJECT

public:
    explicit QuotaManagerRpc(FortManager *fortManager, QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }
    RpcManager *rpcManager() const;

    void initialize() override;

private:
    void setupRpc();

private:
    FortManager *m_fortManager = nullptr;
};

#endif // QUOTAMANAGERRPC_H
