#ifndef CONFZONEMANAGER_H
#define CONFZONEMANAGER_H

#include <QHash>
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

    QString zoneNameById(quint8 zoneId);
    QStringList zoneNamesByMask(quint32 zonesMask);

    virtual bool addOrUpdateZone(Zone &zone);
    virtual bool deleteZone(quint8 zoneId);
    virtual bool updateZoneName(quint8 zoneId, const QString &zoneName);
    virtual bool updateZoneEnabled(quint8 zoneId, bool enabled);

    bool updateZoneResult(const Zone &zone);

    void updateDriverZones(quint32 zonesMask, quint32 enabledMask, quint32 dataSize,
            const QList<QByteArray> &zonesData);

signals:
    void zoneAdded();
    void zoneRemoved(quint8 zoneId);
    void zoneUpdated();

private:
    void setupZoneNamesCache();
    void clearZoneNamesCache();

    bool updateDriverZoneFlag(quint8 zoneId, bool enabled);

private:
    mutable QHash<quint8, QString> m_zoneNamesCache;
};

#endif // CONFZONEMANAGER_H
