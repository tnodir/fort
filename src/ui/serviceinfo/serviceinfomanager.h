#ifndef SERVICEINFOMANAGER_H
#define SERVICEINFOMANAGER_H

#include <QHash>
#include <QObject>

#include <util/ioc/iocservice.h>

#include "serviceinfo.h"

class ServiceInfoMonitor;

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

    ServiceInfoMonitor *startServiceMonitor(const ServiceInfo &info, void *managerHandle);
    void stopServiceMonitor(ServiceInfoMonitor *m);

    void onServiceStateChanged(ServiceInfo::State state);

private:
    bool m_monitorEnabled = false;

    QHash<QString, int> m_serviceGroups;
    QHash<QString, ServiceInfoMonitor *> m_serviceMonitors;
};

#endif // SERVICEINFOMANAGER_H
