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

    DriverWorker *driverWorker() const { return m_driverWorker; }

    QString errorMessage() const;
    bool isDeviceError() const;

    bool isDeviceOpened() const;

    static void reinstallDriver();
    static void uninstallDriver();

signals:
    void errorMessageChanged();
    void isDeviceOpenedChanged();

public slots:
    bool openDevice();
    bool closeDevice();

    bool validate(QByteArray &buf, int size);

    bool writeConf(QByteArray &buf, int size, bool onlyFlags = false);
    bool writeApp(QByteArray &buf, int size, bool remove = false);
    bool writeZones(QByteArray &buf, int size, bool onlyFlags = false);

private:
    void setErrorMessage(const QString &errorMessage);
    void updateError(bool success);

    void setupWorker();
    void abortWorker();

    bool writeData(quint32 code, QByteArray &buf, int size);

    static void executeCommand(const QString &fileName);

private:
    quint32 m_errorCode = 0;

    Device *m_device = nullptr;
    DriverWorker *m_driverWorker = nullptr;
};

#endif // DRIVERMANAGER_H
