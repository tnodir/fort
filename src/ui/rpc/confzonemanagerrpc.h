#ifndef CONFZONEMANAGERRPC_H
#define CONFZONEMANAGERRPC_H

#include <conf/confzonemanager.h>

class RpcManager;

struct ProcessCommandArgs;

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

    static bool processServerCommand(
            const ProcessCommandArgs &p, QVariantList &resArgs, bool &ok, bool &isSendResult);

    static void setupServerSignals(RpcManager *rpcManager);
};

#endif // CONFZONEMANAGERRPC_H
