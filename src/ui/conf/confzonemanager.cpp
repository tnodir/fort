#include "confzonemanager.h"

#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/zone.h>
#include <driver/drivermanager.h>
#include <util/bitutil.h>
#include <util/conf/confbuffer.h>
#include <util/conf/confutil.h>
#include <util/dateutil.h>
#include <util/ioc/ioccontainer.h>

#include "confmanager.h"

namespace {

const QLoggingCategory LC("confZone");

const char *const sqlInsertZone = "INSERT INTO zone(zone_id, name, enabled, custom_url,"
                                  "    source_code, url, form_data, text_inline, mod_time)"
                                  "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9);";

const char *const sqlUpdateZone = "UPDATE zone"
                                  "  SET name = ?2, enabled = ?3, custom_url = ?4,"
                                  "    source_code = ?5, url = ?6,"
                                  "    form_data = ?7, text_inline = ?8, mod_time = ?9"
                                  "  WHERE zone_id = ?1;";

const char *const sqlSelectZoneNameById = "SELECT name FROM zone WHERE zone_id = ?1;";

const char *const sqlSelectZoneIds = "SELECT zone_id FROM zone"
                                     "  WHERE zone_id < ?1 ORDER BY zone_id;";

const char *const sqlDeleteZone = "DELETE FROM zone WHERE zone_id = ?1;";

const char *const sqlDeleteAddressGroupZone =
        "UPDATE address_group"
        "  SET include_zones = include_zones & ~?1,"
        "    exclude_zones = exclude_zones & ~?1"
        "  WHERE (include_zones & ?1) <> 0 || (exclude_zones & ?1) <> 0;";

const char *const sqlDeleteAppZone =
        "UPDATE app"
        "  SET accept_zones = accept_zones & ~?1,"
        "    reject_zones = reject_zones & ~?1"
        "  WHERE (accept_zones & ?1) <> 0 || (reject_zones & ?1) <> 0;";

const char *const sqlDeleteRuleZone =
        "UPDATE rule"
        "  SET accept_zones = accept_zones & ~?1,"
        "    reject_zones = reject_zones & ~?1"
        "  WHERE (accept_zones & ?1) <> 0 || (reject_zones & ?1) <> 0;";

const char *const sqlUpdateZoneName = "UPDATE zone SET name = ?2 WHERE zone_id = ?1;";

const char *const sqlUpdateZoneEnabled = "UPDATE zone SET enabled = ?2 WHERE zone_id = ?1;";

const char *const sqlUpdateZoneResult =
        "UPDATE zone"
        "  SET address_count = ?2, text_checksum = ?3, bin_checksum = ?4,"
        "    source_modtime = ?5, last_run = ?6, last_success = ?7"
        "  WHERE zone_id = ?1;";

bool driverWriteZones(ConfBuffer &confBuf, bool onlyFlags = false)
{
    if (confBuf.hasError()) {
        qCWarning(LC) << "Driver config error:" << confBuf.errorMessage();
        return false;
    }

    auto driverManager = IoC<DriverManager>();
    if (!driverManager->writeZones(confBuf.buffer(), onlyFlags)) {
        qCWarning(LC) << "Update driver error:" << driverManager->errorMessage();
        return false;
    }

    return true;
}

}

ConfZoneManager::ConfZoneManager(QObject *parent) : ConfManagerBase(parent)
{
    setupZoneNamesCache();
}

QString ConfZoneManager::zoneNameById(quint8 zoneId)
{
    QString name = m_zoneNamesCache.value(zoneId);

    if (name.isEmpty()) {
        name = DbQuery(sqliteDb()).sql(sqlSelectZoneNameById).vars({ zoneId }).execute().toString();

        m_zoneNamesCache.insert(zoneId, name);
    }

    return name;
}

QStringList ConfZoneManager::zoneNamesByMask(quint32 zonesMask)
{
    QStringList list;

    while (zonesMask != 0) {
        const int zoneIndex = BitUtil::bitScanForward(zonesMask);
        const quint8 zoneId = zoneIndex + 1;

        list << zoneNameById(zoneId);

        zonesMask ^= (1u << zoneIndex);
    }

    return list;
}

bool ConfZoneManager::addOrUpdateZone(Zone &zone)
{
    bool ok = true;

    beginWriteTransaction();

    const bool isNew = (zone.zoneId == 0);
    if (isNew) {
        zone.zoneId = DbQuery(sqliteDb(), &ok)
                              .sql(sqlSelectZoneIds)
                              .vars({ ConfUtil::zoneMaxCount() })
                              .getFreeId(/*maxId=*/ConfUtil::zoneMaxCount() - 1);
    } else {
        updateDriverZoneFlag(zone.zoneId, zone.enabled);
    }

    if (ok) {
        const QVariantList vars = {
            zone.zoneId,
            zone.zoneName,
            zone.enabled,
            zone.customUrl,
            zone.sourceCode,
            zone.url,
            zone.formData,
            zone.textInline,
            DateUtil::now(),
        };

        DbQuery(sqliteDb(), &ok).sql(isNew ? sqlInsertZone : sqlUpdateZone).vars(vars).executeOk();
    }

    endTransaction(ok);

    if (!ok)
        return false;

    if (isNew) {
        emit zoneAdded();
    } else {
        emit zoneUpdated();
    }

    return true;
}

bool ConfZoneManager::deleteZone(quint8 zoneId)
{
    bool ok = false;

    beginWriteTransaction();

    if (DbQuery(sqliteDb(), &ok).sql(sqlDeleteZone).vars({ zoneId }).executeOk()) {
        const quint32 zoneBit = (quint32(1) << (zoneId - 1));
        const QVariantList vars = { zoneBit };

        // Delete the Zone from Address Groups
        DbQuery(sqliteDb(), &ok).sql(sqlDeleteAddressGroupZone).vars(vars).executeOk();

        // Delete the Zone from Programs
        DbQuery(sqliteDb(), &ok).sql(sqlDeleteAppZone).vars(vars).executeOk();

        // Delete the Zone from Rules
        DbQuery(sqliteDb(), &ok).sql(sqlDeleteRuleZone).vars(vars).executeOk();
    }

    endTransaction(ok);

    if (ok) {
        emit zoneRemoved(zoneId);
    }

    return ok;
}

bool ConfZoneManager::updateZoneName(quint8 zoneId, const QString &zoneName)
{
    bool ok = false;

    beginWriteTransaction();

    const QVariantList vars = { zoneId, zoneName };

    DbQuery(sqliteDb(), &ok).sql(sqlUpdateZoneName).vars(vars).executeOk();

    endTransaction(ok);

    if (ok) {
        emit zoneUpdated();
    }

    return ok;
}

bool ConfZoneManager::updateZoneEnabled(quint8 zoneId, bool enabled)
{
    bool ok = false;

    beginWriteTransaction();

    const QVariantList vars = { zoneId, enabled };

    DbQuery(sqliteDb(), &ok).sql(sqlUpdateZoneEnabled).vars(vars).executeOk();

    endTransaction(ok);

    if (ok) {
        emit zoneUpdated();

        updateDriverZoneFlag(zoneId, enabled);
    }

    return ok;
}

bool ConfZoneManager::updateZoneResult(const Zone &zone)
{
    bool ok = false;

    beginWriteTransaction();

    const QVariantList vars = {
        zone.zoneId,
        zone.addressCount,
        zone.textChecksum,
        zone.binChecksum,
        zone.sourceModTime,
        zone.lastRun,
        zone.lastSuccess,
    };

    DbQuery(sqliteDb(), &ok).sql(sqlUpdateZoneResult).vars(vars).executeOk();

    endTransaction(ok);

    if (ok) {
        emit zoneUpdated();
    }

    return ok;
}

void ConfZoneManager::updateDriverZones(quint32 zonesMask, quint32 enabledMask, quint32 dataSize,
        const QList<QByteArray> &zonesData)
{
    ConfBuffer confBuf;

    confBuf.writeZones(zonesMask, enabledMask, dataSize, zonesData);

    driverWriteZones(confBuf);
}

void ConfZoneManager::setupZoneNamesCache()
{
    connect(this, &ConfZoneManager::zoneRemoved, this, &ConfZoneManager::clearZoneNamesCache);
    connect(this, &ConfZoneManager::zoneUpdated, this, &ConfZoneManager::clearZoneNamesCache);
}

void ConfZoneManager::clearZoneNamesCache()
{
    m_zoneNamesCache.clear();
}

bool ConfZoneManager::updateDriverZoneFlag(quint8 zoneId, bool enabled)
{
    ConfBuffer confBuf;

    confBuf.writeZoneFlag(zoneId, enabled);

    return driverWriteZones(confBuf, /*onlyFlags=*/true);
}
