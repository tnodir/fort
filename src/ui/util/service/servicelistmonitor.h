#ifndef SERVICELISTMONITOR_H
#define SERVICELISTMONITOR_H

#include <QByteArray>
#include <QObject>

class ServiceListMonitor : public QObject
{
    Q_OBJECT

public:
    explicit ServiceListMonitor(QObject *parent = nullptr);
    ~ServiceListMonitor() override;

    void *managerHandle() const { return m_managerHandle; }

    void startMonitor();
    void stopMonitor();

    void onManagerNotify(const QStringList &createdServiceNames);

signals:
    void servicesCreated(const QStringList &serviceNames);

private:
    void *m_managerHandle = nullptr;

    QByteArray m_buffer;
};

#endif // SERVICELISTMONITOR_H
