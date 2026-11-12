#include "confzonemanager.h"

#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/zone.h>
#include <driver/drivermanager.h>
#include <fortglobal.h>
#include <model/zonelistmodel.h>
#include <util/bitutil.h>
#include <util/conf/confbuffer.h>
#include <util/conf/confutil.h>
#include <util/dateutil.h>

#include "confmanager.h"
#include "inioptions.h"

using namespace Fort;

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
                                     "  WHERE zone_id <= ?1 ORDER BY zone_id;";

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

// The address lists' ids: FirewallConf::addressList()
const char *const sqlSelectZoneUsages =
        "WITH addr_list(list_id, include_zones, exclude_zones) AS ("
        "  SELECT (ROW_NUMBER() OVER (ORDER BY addr_group_id) - 1) * 2,"
        "      include_zones, exclude_zones"
        "    FROM address_group"
        ")"
        "SELECT * FROM ("
        "  SELECT ?2 AS category, t.list_id AS id, '' AS name"
        "    FROM addr_list t"
        "    WHERE (t.include_zones & ?1) <> 0"
        "  UNION ALL"
        "  SELECT ?2, t.list_id + 1, ''"
        "    FROM addr_list t"
        "    WHERE (t.exclude_zones & ?1) <> 0"
        "  UNION ALL"
        "  SELECT ?3, t.app_id, t.name"
        "    FROM app t"
        "    WHERE ((t.accept_zones | t.reject_zones) & ?1) <> 0"
        "  UNION ALL"
        "  SELECT ?4 + t.rule_type, t.rule_id, t.name"
        "    FROM rule t"
        "    WHERE ((t.accept_zones | t.reject_zones) & ?1) <> 0"
        ") ORDER BY category, lower(name), id;";

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

    auto driverManager = Fort::driverManager();

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

void ConfZoneManager::setUp()
{
    auto confManager = Fort::dependency<ConfManager>();

    // The imported DB may reuse the ids
    connect(confManager, &ConfManager::imported, this, &ConfZoneManager::clearZoneNamesCache);

    // deleteZone() clears the Zone in the DB: clear it in the loaded conf too (a client's by RPC)
    connect(this, &ConfZoneManager::zoneRemoved, confManager, [=](quint8 zoneId) {
        confManager->conf().removeAddressListsZones(Zone::zoneBit(zoneId));
    });
}

QString ConfZoneManager::zoneNameById(quint8 zoneId)
{
    if (zoneId == 0)
        return {};

    const auto it = m_zoneNamesCache.constFind(zoneId);
    if (it != m_zoneNamesCache.constEnd())
        return it.value();

    const QString name =
            DbQuery(sqliteDb()).sql(sqlSelectZoneNameById).vars({ zoneId }).execute().toString();

    m_zoneNamesCache.insert(zoneId, name);

    return name;
}

QStringList ConfZoneManager::zoneNamesByMask(quint32 zonesMask)
{
    QStringList list;

    while (zonesMask != 0) {
        const int zoneIndex = BitUtil::bitScanForward(zonesMask);
        if (Q_UNLIKELY(zoneIndex == -1))
            break;

        const quint8 zoneId = zoneIndex + 1;

        list << zoneNameById(zoneId);

        zonesMask ^= (1u << zoneIndex);
    }

    return list;
}

ConfUsages ConfZoneManager::zoneUsages(quint8 zoneId) const
{
    return loadUsages(sqlSelectZoneUsages,
            { Zone::zoneBit(zoneId), ConfUsages::CategoryAddressLists, ConfUsages::CategoryApps,
                    ConfUsages::CategoryRules });
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
                              .getFreeId(/*maxId=*/ConfUtil::zoneMaxCount());
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

        updateDnsBlockZones();
    }

    return true;
}

bool ConfZoneManager::deleteZone(quint8 zoneId)
{
    bool ok = false;

    beginWriteTransaction();

    if (DbQuery(sqliteDb(), &ok).sql(sqlDeleteZone).vars({ zoneId }).executeOk()) {
        const QVariantList vars = { Zone::zoneBit(zoneId) };

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

        updateDriverZoneFlag(zoneId, /*enabled=*/false);

        updateDnsBlockZones();
    }

    return ok;
}

bool ConfZoneManager::updateZoneName(quint8 zoneId, const QString &zoneName)
{
    if (!executeWrite(sqlUpdateZoneName, { zoneId, zoneName }))
        return false;

    emit zoneUpdated();

    return true;
}

bool ConfZoneManager::updateZoneEnabled(quint8 zoneId, bool enabled)
{
    if (!executeWrite(sqlUpdateZoneEnabled, { zoneId, enabled }))
        return false;

    emit zoneUpdated();

    updateDriverZoneFlag(zoneId, enabled);

    return true;
}

bool ConfZoneManager::updateZoneResult(const Zone &zone)
{
    const QVariantList vars = {
        zone.zoneId,
        zone.addressCount,
        zone.textChecksum,
        zone.binChecksum,
        zone.sourceModTime,
        zone.lastRun,
        zone.lastSuccess,
    };

    if (!executeWrite(sqlUpdateZoneResult, vars))
        return false;

    emit zoneUpdated();

    return true;
}

void ConfZoneManager::updateDriverZones(quint32 zonesMask, quint32 enabledMask, quint32 dataSize,
        const QList<QByteArray> &zonesData)
{
    ConfBuffer confBuf;

    confBuf.writeZones(zonesMask, enabledMask, dataSize, zonesData);

    driverWriteZones(confBuf);
}

bool ConfZoneManager::updateDriverZoneFlag(quint8 zoneId, bool enabled)
{
    ConfBuffer confBuf;

    confBuf.writeZoneFlag(zoneId, enabled);

    return driverWriteZones(confBuf, /*onlyFlags=*/true);
}

// The Block Zones are of the Hosts types only: the Zone's source may be changed, the removed Zone's
// id can be reused
void ConfZoneManager::updateDnsBlockZones()
{
    auto &ini = Fort::ini();

    const quint32 blockZones = ini.dnsBlockZones();
    if (blockZones == 0)
        return;

    const quint32 hostBlockZones = blockZones & zoneListModel()->hostZonesMask();
    if (hostBlockZones == blockZones)
        return;

    ini.setDnsBlockZones(hostBlockZones);

    confManager()->saveIniChanged();
}

void ConfZoneManager::setupZoneNamesCache()
{
    connect(this, &ConfZoneManager::zoneAdded, this, &ConfZoneManager::clearZoneNamesCache);
    connect(this, &ConfZoneManager::zoneRemoved, this, &ConfZoneManager::clearZoneNamesCache);
    connect(this, &ConfZoneManager::zoneUpdated, this, &ConfZoneManager::clearZoneNamesCache);
}

void ConfZoneManager::clearZoneNamesCache()
{
    m_zoneNamesCache.clear();
}
