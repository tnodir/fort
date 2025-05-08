#ifndef STATMANAGERRPC_H
#define STATMANAGERRPC_H

#include <control/control_types.h>
#include <stat/statmanager.h>

class RpcManager;

class StatManagerRpc : public StatManager
{
    Q_OBJECT

public:
    explicit StatManagerRpc(const QString &filePath, QObject *parent = nullptr);

    void setConf(const FirewallConf * /*conf*/) override { }

    bool deleteStatApp(qint64 appId) override;

    bool resetAppTrafTotals() override;

    bool exportMasterBackup(const QString &path) override;
    bool importMasterBackup(const QString &path) override;

    static bool processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);

    static void setupServerSignals(RpcManager *rpcManager);

public slots:
    bool clearTraffic() override;
};

#endif // STATMANAGERRPC_H
