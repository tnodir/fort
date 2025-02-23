#ifndef CONFZONEMANAGER_H
#define CONFZONEMANAGER_H

#include <QObject>

#include <util/classhelpers.h>
#include <util/ioc/iocservice.h>

#include "confmanagerbase.h"

class Zone;

class ConfZoneManager : public ConfManagerBase, public IocService
{
    Q_OBJECT

public:
    explicit ConfZoneManager(QObject *parent = nullptr);
    CLASS_DELETE_COPY_MOVE(ConfZoneManager)

    QString zoneNameById(int zoneId);

    virtual bool addOrUpdateZone(Zone &zone);
    virtual bool deleteZone(int zoneId);
    virtual bool updateZoneName(int zoneId, const QString &zoneName);
    virtual bool updateZoneEnabled(int zoneId, bool enabled);

    bool updateZoneResult(const Zone &zone);

    void updateDriverZones(quint32 zonesMask, quint32 enabledMask, quint32 dataSize,
            const QList<QByteArray> &zonesData);

signals:
    void zoneAdded();
    void zoneRemoved(int zoneId);
    void zoneUpdated();

private:
    bool updateDriverZoneFlag(int zoneId, bool enabled);
};

#endif // CONFZONEMANAGER_H
