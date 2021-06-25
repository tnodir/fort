#ifndef SERVICEINFO_H
#define SERVICEINFO_H

#include <QObject>

class ServiceInfo
{
public:
    int groupIndex = -1;

    quint32 processId = 0;
    quint64 id = 0;

    QString serviceName;
    QString displayName;
};

#endif // SERVICEINFO_H
