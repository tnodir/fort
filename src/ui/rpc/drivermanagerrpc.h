#ifndef DRIVERMANAGERRPC_H
#define DRIVERMANAGERRPC_H

#include "../driver/drivermanager.h"

class FortManager;
class RpcManager;

class DriverManagerRpc : public DriverManager
{
    Q_OBJECT

public:
    explicit DriverManagerRpc(FortManager *fortManager, QObject *parent = nullptr);

    bool isDeviceOpened() const override { return m_isDeviceOpened; }
    void setIsDeviceOpened(bool v);

    FortManager *fortManager() const { return m_fortManager; }
    RpcManager *rpcManager() const;

    void reinstallDriver() override;
    void uninstallDriver() override;

    void updateState(bool isDeviceOpened, quint32 errorCode);

public slots:
    bool openDevice() override;
    bool closeDevice() override;

private:
    bool m_isDeviceOpened : 1;

    FortManager *m_fortManager = nullptr;
};

#endif // DRIVERMANAGERRPC_H
