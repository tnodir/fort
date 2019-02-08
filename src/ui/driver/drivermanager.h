#ifndef DRIVERMANAGER_H
#define DRIVERMANAGER_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(Device)
QT_FORWARD_DECLARE_CLASS(DriverWorker)
QT_FORWARD_DECLARE_CLASS(FirewallConf)

class DriverManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit DriverManager(QObject *parent = nullptr);
    ~DriverManager() override;

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

    bool writeData(quint32 code, QByteArray &buf, int size);

private:
    Device *m_device;
    DriverWorker *m_driverWorker;

    QString m_errorMessage;
};

#endif // DRIVERMANAGER_H
