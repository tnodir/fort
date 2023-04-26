#ifndef SERVICEINFO_H
#define SERVICEINFO_H

#include <QObject>

class ServiceInfo
{
public:
    enum RegTrackFlag { RegImagePath = 0x01, RegType = 0x02 };

    enum State {
        StateActive = 0x01, // SERVICE_ACTIVE
        StateInactive = 0x02, // SERVICE_INACTIVE
        StateAll = (StateActive | StateInactive), // SERVICE_STATE_ALL
    };

    bool isTracked() const { return trackFlags != 0; }

    quint32 trackFlags = 0;
    quint32 processId = 0;
    QString serviceName;
    QString displayName;
};

#endif // SERVICEINFO_H
