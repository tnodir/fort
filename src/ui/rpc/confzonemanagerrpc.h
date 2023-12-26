#ifndef CONFZONEMANAGERRPC_H
#define CONFZONEMANAGERRPC_H

#include <conf/confzonemanager.h>

class RpcManager;

class ConfZoneManagerRpc : public ConfZoneManager
{
    Q_OBJECT

public:
    explicit ConfZoneManagerRpc(QObject *parent = nullptr);

    bool addOrUpdateZone(Zone &zone) override;
    bool deleteZone(int zoneId) override;
    bool updateZoneName(int zoneId, const QString &zoneName) override;
    bool updateZoneEnabled(int zoneId, bool enabled) override;

    static QVariantList zoneToVarList(const Zone &zone);
    static Zone varListToZone(const QVariantList &v);
};

#endif // CONFZONEMANAGERRPC_H
