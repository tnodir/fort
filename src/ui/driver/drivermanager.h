#ifndef DRIVERMANAGER_H
#define DRIVERMANAGER_H

#include <QObject>

#include "../util/classhelpers.h"
#include "../util/ioc/iocservice.h"

class Device;
class DriverWorker;

class DriverManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit DriverManager(QObject *parent = nullptr, bool useDevice = true);
    ~DriverManager() override;
    CLASS_DELETE_COPY_MOVE(DriverManager)

    Device *device() const { return m_device; }
    DriverWorker *driverWorker() const { return m_driverWorker; }

    quint32 errorCode() const { return m_errorCode; }
    QString errorMessage() const;
    bool isDeviceError() const;

    virtual bool isDeviceOpened() const;

    void setUp() override;

    bool reinstallDriver();
    bool uninstallDriver();

signals:
    void errorCodeChanged();
    void isDeviceOpenedChanged();

public slots:
    virtual bool openDevice();
    virtual bool closeDevice();

    bool validate(QByteArray &buf, int size);

    bool writeConf(QByteArray &buf, int size, bool onlyFlags = false);
    bool writeApp(QByteArray &buf, int size, bool remove = false);
    bool writeZones(QByteArray &buf, int size, bool onlyFlags = false);

protected:
    void setErrorCode(quint32 v);

private:
    void updateErrorCode(bool success);

    void setupWorker();
    void abortWorker();

    bool writeData(quint32 code, QByteArray &buf, int size);

    static bool executeCommand(const QString &fileName);

private:
    quint32 m_errorCode = 0;

    Device *m_device = nullptr;
    DriverWorker *m_driverWorker = nullptr;
};

#endif // DRIVERMANAGER_H
