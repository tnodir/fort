#ifndef DRIVERMANAGER_H
#define DRIVERMANAGER_H

#include <QObject>
#include <QPointer>

#include <util/classhelpers.h>
#include <util/device.h>
#include <util/ioc/iocservice.h>

#include "driverworker.h"

class DriverManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit DriverManager(QObject *parent = nullptr, bool useDevice = true);
    ~DriverManager() override;
    CLASS_DELETE_COPY_MOVE(DriverManager)

    Device &device() { return m_device; }
    const Device &device() const { return m_device; }

    DriverWorker *driverWorker() const { return m_driverWorker.data(); }

    quint32 errorCode() const { return m_errorCode; }
    QString errorMessage() const;
    bool isDeviceError() const;

    virtual bool isDeviceOpened() const;

    void setUp() override;

    static bool checkReinstallDriver();
    static bool reinstallDriver();
    static bool uninstallDriver();

signals:
    void errorCodeChanged();
    void isDeviceOpenedChanged();

public slots:
    virtual bool openDevice();
    virtual bool closeDevice();

    bool validate(QByteArray &buf);

    bool writeServices(QByteArray &buf);
    bool writeConf(QByteArray &buf, bool onlyFlags = false);
    bool writeApp(QByteArray &buf, bool remove = false);
    bool writeZones(QByteArray &buf, bool onlyFlags = false);
    bool writeRules(QByteArray &buf, bool onlyFlags = false);
    bool writeGroups(QByteArray &buf, bool onlyFlags = false);
    bool writeSpeedLimits(QByteArray &buf, bool onlyFlags = false);

    virtual bool readSpeedLimitStatus(QByteArray &buf);

protected:
    void setErrorCode(quint32 v);

private:
    void updateErrorCode(bool success);

    void setupWorker();
    void closeWorker();

    bool writeData(quint32 code, QByteArray &buf);
    bool ioctlData(quint32 code, QByteArray &inBuf, QByteArray &outBuf);

    static bool executeCommand(const QString &fileName);

private:
    quint32 m_errorCode = 0;

    Device m_device;

    QPointer<DriverWorker> m_driverWorker;
};

#endif // DRIVERMANAGER_H
