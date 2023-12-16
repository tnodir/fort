#ifndef DRIVERMANAGERRPC_H
#define DRIVERMANAGERRPC_H

#include <driver/drivermanager.h>

class DriverManagerRpc : public DriverManager
{
    Q_OBJECT

public:
    explicit DriverManagerRpc(QObject *parent = nullptr);

    bool isDeviceOpened() const override { return m_isDeviceOpened; }
    void setIsDeviceOpened(bool v);

    void setUp() override { }

    void updateState(quint32 errorCode, bool isDeviceOpened);

public slots:
    bool openDevice() override;
    bool closeDevice() override;

private:
    bool m_isDeviceOpened : 1 = false;
};

#endif // DRIVERMANAGERRPC_H
