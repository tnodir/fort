#ifndef SERVICEINFOMANAGER_H
#define SERVICEINFOMANAGER_H

#include <QHash>

#include <util/ioc/iocservice.h>
#include <util/service/serviceinfo.h>

class ServiceListMonitor;
class ServiceMonitor;

class ServiceInfoManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit ServiceInfoManager(QObject *parent = nullptr);

    void setUp() override;

    static QVector<ServiceInfo> loadServiceInfoList(
            ServiceInfo::Type serviceType = ServiceInfo::TypeWin32,
            ServiceInfo::State state = ServiceInfo::StateAll, bool displayName = true,
            int *runningServicesCount = nullptr);

    static QString getSvcHostServiceDll(const QString &serviceName);

signals:
    void servicesStarted(const QVector<ServiceInfo> &services, int runningServicesCount);

public slots:
    virtual void trackService(const QString &serviceName);
    virtual void revertService(const QString &serviceName);

    void monitorServices(const QVector<ServiceInfo> &serviceInfoList);

protected:
    virtual void setupServiceListMonitor();
    void setupServiceMonitor(const QString &serviceName);

    bool isServiceMonitoring(const QString &serviceName) const;
    void startServiceMonitor(ServiceMonitor *serviceMonitor);
    void stopServiceMonitor(ServiceMonitor *serviceMonitor);

private:
    void onServicesCreated(const QStringList &serviceNames);
    void onServiceStateChanged(ServiceMonitor *serviceMonitor);
    void onServiceStarted(ServiceMonitor *serviceMonitor);

private:
    ServiceListMonitor *m_serviceListMonitor = nullptr;
    QHash<QString, ServiceMonitor *> m_serviceMonitors;
};

#endif // SERVICEINFOMANAGER_H
