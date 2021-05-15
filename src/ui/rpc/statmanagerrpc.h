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

    bool deleteStatApp(qint64 appId) override;

    bool deleteConn(qint64 rowIdTo, bool blocked) override;
    bool deleteConnAll() override;

    bool resetAppTrafTotals() override;

public slots:
    bool clearTraffic() override;

    void onConnBlockAdded();
    void onConnRemoved();

private:
    FortManager *m_fortManager = nullptr;
};

#endif // STATMANAGERRPC_H
