#ifndef DRIVERMANAGER_H
#define DRIVERMANAGER_H

#include <QObject>

class Device;
class DriverWorker;
class FirewallConf;

class DriverManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit DriverManager(QObject *parent = nullptr);
    virtual ~DriverManager();

    DriverWorker *driverWorker() const { return m_driverWorker; }

    QString errorMessage() const { return m_errorMessage; }

    bool isDeviceOpened() const;

signals:
    void errorMessageChanged();

public slots:
    bool openDevice();
    bool closeDevice();

    bool writeConf(const FirewallConf &conf);
    bool writeConfFlags(const FirewallConf &conf);

private:
    void setErrorMessage(const QString &errorMessage);

    void setupWorker();
    void abortWorker();

    bool writeData(int code, QByteArray &buf, int size);

private:
    Device *m_device;
    DriverWorker *m_driverWorker;

    QString m_errorMessage;
};

#endif // DRIVERMANAGER_H
