#ifndef SERVICEINFOMANAGER_H
#define SERVICEINFOMANAGER_H

#include <QHash>
#include <QObject>

#include <util/ioc/iocservice.h>

#include "serviceinfo.h"

class ServiceInfoMonitor;
class ServiceListMonitor;

class ServiceInfoManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit ServiceInfoManager(QObject *parent = nullptr);
    ~ServiceInfoManager() override;

    bool monitorEnabled() const { return m_monitorEnabled; }
    void setMonitorEnabled(bool v);

    int groupIndexByName(const QString &name) const;

    static QVector<ServiceInfo> loadServiceInfoList();

signals:
    void servicesChanged();

private:
    void setupServiceMonitors();
    void clearServiceMonitors();

    void startServiceMonitor(
            const QString &name, quint32 processId = 0, void *managerHandle = nullptr);
    void stopServiceMonitor(ServiceInfoMonitor *m);

    void startServiceListMonitor(void *managerHandle = nullptr);
    void stopServiceListMonitor();

    void onServiceStateChanged(ServiceInfo::State state);
    void onServiceCreated(const QStringList &nameList);

private:
    bool m_monitorEnabled = false;

    QHash<QString, int> m_serviceGroups;
    QHash<QString, ServiceInfoMonitor *> m_serviceMonitors;
    ServiceListMonitor *m_serviceListMonitor = nullptr;
};

#endif // SERVICEINFOMANAGER_H
