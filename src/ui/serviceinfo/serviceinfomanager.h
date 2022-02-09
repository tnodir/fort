#ifndef SERVICEINFOMANAGER_H
#define SERVICEINFOMANAGER_H

#include <util/ioc/iocservice.h>

#include "serviceinfo.h"

class ServiceInfoManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit ServiceInfoManager(QObject *parent = nullptr);

    static QVector<ServiceInfo> loadServiceInfoList(
            ServiceInfo::State state = ServiceInfo::StateAlive);

    static QString getSvcHostServiceDll(const QString &serviceName);

public slots:
    virtual void trackService(const QString &serviceName);
    virtual void revertService(const QString &serviceName);
};

#endif // SERVICEINFOMANAGER_H
