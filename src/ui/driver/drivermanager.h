#ifndef DRIVERMANAGER_H
#define DRIVERMANAGER_H

#include <QObject>

#include "../util/classhelpers.h"

class ConfManager;
class Device;
class DriverWorker;
class EnvManager;
class FirewallConf;

class DriverManager : public QObject
{
    Q_OBJECT

public:
    explicit DriverManager(QObject *parent = nullptr);
    ~DriverManager() override;
    CLASS_DELETE_COPY_MOVE(DriverManager)

    Device *device() const { return m_device; }
    DriverWorker *driverWorker() const { return m_driverWorker; }

    quint32 errorCode() const { return m_errorCode; }
    QString errorMessage() const;
    bool isDeviceError() const;

    virtual bool isDeviceOpened() const;

    virtual void initialize();

    virtual void reinstallDriver();
    virtual void uninstallDriver();

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

    virtual void abortWorker();

private:
    void updateErrorCode(bool success);

    bool writeData(quint32 code, QByteArray &buf, int size);

    static void executeCommand(const QString &fileName);

private:
    quint32 m_errorCode = 0;

    Device *m_device = nullptr;
    DriverWorker *m_driverWorker = nullptr;
};

#endif // DRIVERMANAGER_H
