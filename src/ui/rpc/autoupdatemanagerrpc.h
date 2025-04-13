#ifndef AUTOUPDATEMANAGERRPC_H
#define AUTOUPDATEMANAGERRPC_H

#include <control/control_types.h>
#include <manager/autoupdatemanager.h>

class RpcManager;

class AutoUpdateManagerRpc : public AutoUpdateManager
{
    Q_OBJECT

public:
    explicit AutoUpdateManagerRpc(const QString &updatePath, QObject *parent = nullptr);

    int bytesReceived() const override { return m_bytesReceived; }
    void setBytesReceived(int v);

    void updateState(AutoUpdateManager::Flags flags, int bytesReceived, const QString &fileName);

    static QVariantList updateState_args();

    static bool processInitClient(ControlWorker *w);

    static bool processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);

    static void setupServerSignals(RpcManager *rpcManager);

public slots:
    bool startDownload() override;

    bool runInstaller() override;

protected:
    void setupManager() override;

private:
    void setupClientSignals();

private:
    int m_bytesReceived = 0;
};

#endif // AUTOUPDATEMANAGERRPC_H
