#ifndef ZONESCONTROLLER_H
#define ZONESCONTROLLER_H

#include <form/basecontroller.h>

class Zone;
class ZoneListModel;

class ZonesController : public BaseController
{
    Q_OBJECT

public:
    explicit ZonesController(QObject *parent = nullptr);

    ZoneListModel *zoneListModel() const;

public slots:
    bool addOrUpdateZone(Zone &zone);
    void deleteZone(int zoneId);
    bool updateZoneName(int zoneId, const QString &zoneName);
};

#endif // ZONESCONTROLLER_H
