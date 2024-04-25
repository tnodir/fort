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

    void setUp() override;

    void updateState(bool isDownloaded, bool isDownloading, int bytesReceived);

    static QVariantList updateState_args();

    static bool processInitClient(ControlWorker *w);

    static bool processServerCommand(
            const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult);

    static void setupServerSignals(RpcManager *rpcManager);

public slots:
    bool startDownload() override;

private:
    void setupClientSignals();

private:
    int m_bytesReceived = 0;
};

#endif // AUTOUPDATEMANAGERRPC_H
