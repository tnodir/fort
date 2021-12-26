#ifndef SERVICEINFO_H
#define SERVICEINFO_H

#include <QObject>

class ServiceInfo
{
public:
    enum State {
        StateActive = 0x01,
        StateInactive = 0x02,
        StateDeleted = 0x04,
    };

    quint32 processId = 0;
    QString serviceName;
    QString displayName;
};

#endif // SERVICEINFO_H
