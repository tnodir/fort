#include "confzonemanager.h"

#include <QLoggingCategory>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/zone.h>
#include <driver/drivermanager.h>
#include <manager/windowmanager.h>
#include <util/conf/confutil.h>
#include <util/ioc/ioccontainer.h>

#include "confmanager.h"

namespace {

const QLoggingCategory LC("confZone");

const char *const sqlInsertZone = "INSERT INTO zone(zone_id, name, enabled, custom_url,"
                                  "    source_code, url, form_data, text_inline)"
                                  "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8);";

const char *const sqlUpdateZone = "UPDATE zone"
                                  "  SET name = ?2, enabled = ?3, custom_url = ?4,"
                                  "    source_code = ?5, url = ?6,"
                                  "    form_data = ?7, text_inline = ?8"
                                  "  WHERE zone_id = ?1;";

const char *const sqlSelectZoneIds = "SELECT zone_id FROM zone ORDER BY zone_id;";

const char *const sqlDeleteZone = "DELETE FROM zone WHERE zone_id = ?1;";

const char *const sqlDeleteAddressGroupZone = "UPDATE address_group"
                                              "  SET include_zones = include_zones & ?1,"
                                              "    exclude_zones = exclude_zones & ?1;";

const char *const sqlDeleteAppZone = "UPDATE app"
                                     "  SET accept_zones = accept_zones & ?1,"
                                     "    reject_zones = reject_zones & ?1;";

const char *const sqlUpdateZoneName = "UPDATE zone SET name = ?2 WHERE zone_id = ?1;";

const char *const sqlUpdateZoneEnabled = "UPDATE zone SET enabled = ?2 WHERE zone_id = ?1;";

const char *const sqlUpdateZoneResult =
        "UPDATE zone"
        "  SET address_count = ?2, text_checksum = ?3, bin_checksum = ?4,"
        "    source_modtime = ?5, last_run = ?6, last_success = ?7"
        "  WHERE zone_id = ?1;";

int getFreeZoneId(SqliteDb *sqliteDb)
{
    int zoneId = 1;

    SqliteStmt stmt;
    if (stmt.prepare(sqliteDb->db(), sqlSelectZoneIds)) {
        while (stmt.step() == SqliteStmt::StepRow) {
            const int id = stmt.columnInt(0);
            if (id > zoneId)
                break;

            zoneId = id + 1;
        }
    }

    return zoneId;
}

void showErrorMessage(const QString &errorMessage)
{
    IoC<WindowManager>()->showErrorBox(
            errorMessage, ConfZoneManager::tr("Zone Configuration Error"));
}

bool driverWriteZones(ConfUtil &confUtil, QByteArray &buf, int entrySize, bool onlyFlags = false)
{
    if (entrySize == 0) {
        showErrorMessage(confUtil.errorMessage());
        return false;
    }

    auto driverManager = IoC<DriverManager>();
    if (!driverManager->writeZones(buf, entrySize, onlyFlags)) {
        showErrorMessage(driverManager->errorMessage());
        return false;
    }

    return true;
}

}

ConfZoneManager::ConfZoneManager(QObject *parent) : QObject(parent) { }

ConfManager *ConfZoneManager::confManager() const
{
    return m_confManager;
}

SqliteDb *ConfZoneManager::sqliteDb() const
{
    return confManager()->sqliteDb();
}

void ConfZoneManager::setUp()
{
    m_confManager = IoC()->setUpDependency<ConfManager>();
}

bool ConfZoneManager::addOrUpdateZone(Zone &zone)
{
    bool ok = false;
    const bool isNew = (zone.zoneId == 0);

    if (isNew) {
        zone.zoneId = getFreeZoneId(sqliteDb());
    } else {
        updateDriverZoneFlag(zone.zoneId, zone.enabled);
    }

    const auto vars = QVariantList()
            << zone.zoneId << zone.zoneName << zone.enabled << zone.customUrl << zone.sourceCode
            << zone.url << zone.formData << zone.textInline;

    sqliteDb()->executeEx(isNew ? sqlInsertZone : sqlUpdateZone, vars, 0, &ok);

    checkEndTransaction(ok);

    if (!ok)
        return false;

    if (isNew) {
        emit zoneAdded();
    } else {
        emit zoneUpdated();
    }

    return true;
}

bool ConfZoneManager::deleteZone(int zoneId)
{
    bool ok = false;

    beginTransaction();

    sqliteDb()->executeEx(sqlDeleteZone, { zoneId }, 0, &ok);
    if (ok) {
        const quint32 zoneUnMask = ~(quint32(1) << (zoneId - 1));

        // Delete the Zone from Address Groups
        sqliteDb()->executeEx(sqlDeleteAddressGroupZone, { qint64(zoneUnMask) }, 0, &ok);

        // Delete the Zone from Programs
        sqliteDb()->executeEx(sqlDeleteAppZone, { qint64(zoneUnMask) }, 0, &ok);
    }

    commitTransaction(ok);

    if (ok) {
        emit zoneRemoved(zoneId);
    }

    return ok;
}

bool ConfZoneManager::updateZoneName(int zoneId, const QString &zoneName)
{
    bool ok = false;

    const auto vars = QVariantList() << zoneId << zoneName;

    sqliteDb()->executeEx(sqlUpdateZoneName, vars, 0, &ok);

    checkEndTransaction(ok);

    if (ok) {
        emit zoneUpdated();
    }

    return ok;
}

bool ConfZoneManager::updateZoneEnabled(int zoneId, bool enabled)
{
    if (!updateDriverZoneFlag(zoneId, enabled))
        return false;

    bool ok = false;

    const auto vars = QVariantList() << zoneId << enabled;

    sqliteDb()->executeEx(sqlUpdateZoneEnabled, vars, 0, &ok);

    checkEndTransaction(ok);

    if (ok) {
        emit zoneUpdated();
    }

    return ok;
}

bool ConfZoneManager::updateZoneResult(const Zone &zone)
{
    bool ok = false;

    const auto vars = QVariantList()
            << zone.zoneId << zone.addressCount << zone.textChecksum << zone.binChecksum
            << zone.sourceModTime << zone.lastRun << zone.lastSuccess;

    sqliteDb()->executeEx(sqlUpdateZoneResult, vars, 0, &ok);

    checkEndTransaction(ok);

    if (ok) {
        emit zoneUpdated();
    }

    return ok;
}

void ConfZoneManager::updateDriverZones(quint32 zonesMask, quint32 enabledMask, quint32 dataSize,
        const QList<QByteArray> &zonesData)
{
    ConfUtil confUtil;
    QByteArray buf;

    const int entrySize = confUtil.writeZones(zonesMask, enabledMask, dataSize, zonesData, buf);

    driverWriteZones(confUtil, buf, entrySize);
}

bool ConfZoneManager::updateDriverZoneFlag(int zoneId, bool enabled)
{
    ConfUtil confUtil;
    QByteArray buf;

    const int entrySize = confUtil.writeZoneFlag(zoneId, enabled, buf);

    return driverWriteZones(confUtil, buf, entrySize, /*onlyFlags=*/true);
}

bool ConfZoneManager::beginTransaction()
{
    return sqliteDb()->beginTransaction();
}

bool ConfZoneManager::commitTransaction(bool ok)
{
    ok = sqliteDb()->endTransaction(ok);

    return checkEndTransaction(ok);
}

bool ConfZoneManager::checkEndTransaction(bool ok)
{
    if (!ok) {
        showErrorMessage(sqliteDb()->errorMessage());
    }

    return ok;
}
