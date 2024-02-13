#ifndef STATMANAGERRPC_H
#define STATMANAGERRPC_H

#include <stat/statmanager.h>

class RpcManager;

struct ProcessCommandArgs;

class StatManagerRpc : public StatManager
{
    Q_OBJECT

public:
    explicit StatManagerRpc(const QString &filePath, QObject *parent = nullptr);

    void setConf(const FirewallConf * /*conf*/) override { }

    bool deleteStatApp(qint64 appId) override;

    bool resetAppTrafTotals() override;

    static bool processServerCommand(
            const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult);

    static void setupServerSignals(RpcManager *rpcManager);

public slots:
    bool clearTraffic() override;
};

#endif // STATMANAGERRPC_H
