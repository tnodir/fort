#ifndef AUTOUPDATEMANAGERRPC_H
#define AUTOUPDATEMANAGERRPC_H

#include <manager/autoupdatemanager.h>

class ControlWorker;
class RpcManager;

struct ProcessCommandArgs;

class AutoUpdateManagerRpc : public AutoUpdateManager
{
    Q_OBJECT

public:
    explicit AutoUpdateManagerRpc(const QString &cachePath, QObject *parent = nullptr);

    int bytesReceived() const override { return m_bytesReceived; }
    void setBytesReceived(int v);

    void updateState(AutoUpdateManager::Flags flags, int bytesReceived, const QString &fileName);

    static QVariantList updateState_args();

    static bool processInitClient(ControlWorker *w);

    static bool processServerCommand(
            const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult);

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
