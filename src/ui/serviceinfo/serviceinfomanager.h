#ifndef SERVICEINFOMANAGER_H
#define SERVICEINFOMANAGER_H

#include <QObject>

#include "../util/ioc/iocservice.h"
#include "serviceinfo.h"

class ServiceInfoManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit ServiceInfoManager(QObject *parent = nullptr);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool v);

    QVector<ServiceInfo> &services() { return m_services; }
    const QVector<ServiceInfo> &services() const { return m_services; }

    const ServiceInfo &serviceInfoAt(int index) const;

signals:
    void servicesChanged();

private:
    void updateWorker();
    void startWorker();
    void stopWorker();

    void updateServices();
    void loadServices();
    void loadServiceInfos();

private:
    bool m_enabled = false;

    QVector<ServiceInfo> m_services;
};

#endif // SERVICEINFOMANAGER_H
