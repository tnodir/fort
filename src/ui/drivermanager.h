#ifndef DRIVERMANAGER_H
#define DRIVERMANAGER_H

#include <QObject>

class Device;
class FirewallConf;

class DriverManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit DriverManager(QObject *parent = nullptr);

    QString errorMessage() const { return m_errorMessage; }

signals:
    void errorMessageChanged();

public slots:
    bool openDevice();

    bool writeConf(const FirewallConf &conf);
    bool writeConfFlags(const FirewallConf &conf);

private:
    void setErrorMessage(const QString &errorMessage);

    bool writeData(int code, QByteArray &buf, int size);

private:
    Device *m_device;

    QString m_errorMessage;
};

#endif // DRIVERMANAGER_H
