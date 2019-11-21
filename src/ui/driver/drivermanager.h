#ifndef DRIVERMANAGER_H
#define DRIVERMANAGER_H

#include <QObject>

#include "../util/classhelpers.h"

QT_FORWARD_DECLARE_CLASS(Device)
QT_FORWARD_DECLARE_CLASS(DriverWorker)
QT_FORWARD_DECLARE_CLASS(EnvManager)
QT_FORWARD_DECLARE_CLASS(FirewallConf)

class DriverManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(bool isDeviceOpened READ isDeviceOpened NOTIFY isDeviceOpenedChanged)

public:
    explicit DriverManager(QObject *parent = nullptr);
    ~DriverManager() override;
    CLASS_DELETE_COPY_MOVE(DriverManager)

    DriverWorker *driverWorker() const { return m_driverWorker; }

    QString errorMessage() const { return m_errorMessage; }

    bool isDeviceOpened() const;

    static void reinstallDriver();
    static void uninstallDriver();

signals:
    void errorMessageChanged();
    void isDeviceOpenedChanged();

public slots:
    bool openDevice();
    bool closeDevice();

    bool validate();

    bool writeConf(const FirewallConf &conf, EnvManager &envManager);
    bool writeConfFlags(const FirewallConf &conf);

private:
    void setErrorMessage(const QString &errorMessage);

    void setupWorker();
    void abortWorker();

    bool writeData(quint32 code, QByteArray &buf, int size);

    static void executeCommand(const QString &fileName);

private:
    Device *m_device;
    DriverWorker *m_driverWorker;

    QString m_errorMessage;
};

#endif // DRIVERMANAGER_H
