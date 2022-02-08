#ifndef SERVICEINFOMANAGER_H
#define SERVICEINFOMANAGER_H

#include "serviceinfo.h"

class ServiceInfoManager
{
public:
    static QVector<ServiceInfo> loadServiceInfoList(
            ServiceInfo::State state = ServiceInfo::StateAlive);

    static QString getSvcHostServiceDll(const QString &serviceName);
};

#endif // SERVICEINFOMANAGER_H
