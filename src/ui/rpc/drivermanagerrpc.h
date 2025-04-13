#ifndef DRIVERMANAGERRPC_H
#define DRIVERMANAGERRPC_H

#include <control/control_types.h>
#include <driver/drivermanager.h>

class ControlWorker;
class RpcManager;

class DriverManagerRpc : public DriverManager
{
    Q_OBJECT

public:
    explicit DriverManagerRpc(QObject *parent = nullptr);

    bool isDeviceOpened() const override { return m_isDeviceOpened; }
    void setIsDeviceOpened(bool v);

    void setUp() override { }

    void updateState(quint32 errorCode, bool isDeviceOpened);

    static QVariantList updateState_args();

    static bool processInitClient(ControlWorker *w);

    static bool processServerCommand(const ProcessCommandArgs &p, ProcessCommandResult &r);

    static void setupServerSignals(RpcManager *rpcManager);

public slots:
    bool openDevice() override;
    bool closeDevice() override;

private:
    bool m_isDeviceOpened : 1 = false;
};

#endif // DRIVERMANAGERRPC_H
