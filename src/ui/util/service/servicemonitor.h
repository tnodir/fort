#ifndef SERVICEMONITOR_H
#define SERVICEMONITOR_H

#include <QByteArray>
#include <QObject>

class ServiceMonitor : public QObject
{
    Q_OBJECT

public:
    enum ServiceState : qint8 {
        ServiceStateUnknown = 0,
        ServiceRunning,
        ServiceDeleting,
    };
    Q_ENUM(ServiceState)

    explicit ServiceMonitor(const QString &serviceName, QObject *parent = nullptr);
    ~ServiceMonitor() override;

    ServiceState state() const { return m_state; }
    qint32 processId() const { return m_processId; }
    void *serviceHandle() const { return m_serviceHandle; }
    const QString &serviceName() const { return m_serviceName; }

    void startMonitor(void *managerHandle);
    void stopMonitor();

    void onServiceNotify(quint32 notificationTriggered, qint32 processId);

signals:
    void stateChanged();

private:
    ServiceState m_state = ServiceStateUnknown;
    qint32 m_processId = 0;

    void *m_serviceHandle = nullptr;

    const QString m_serviceName;

    QByteArray m_buffer;
};

#endif // SERVICEMONITOR_H
