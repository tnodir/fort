#include "confspeedlimitmanager.h"

#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/speedlimit.h>
#include <driver/drivermanager.h>
#include <util/conf/confbuffer.h>
#include <util/conf/confutil.h>
#include <util/dateutil.h>
#include <util/ioc/ioccontainer.h>

#include "confmanager.h"

namespace {

const QLoggingCategory LC("confSpeedLimit");

const char *const sqlInsertSpeedLimit = "INSERT INTO speed_limit(limit_id, enabled, inbound, name,"
                                        "    kbps, packet_loss, latency, bufsize, mod_time)"
                                        "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9);";

const char *const sqlUpdateSpeedLimit = "UPDATE speed_limit"
                                        "  SET enabled = ?2, inbound = ?3, name = ?4, kbps = ?5,"
                                        "    packet_loss = ?6, latency = ?7,"
                                        "    bufsize = ?8, mod_time = ?9"
                                        "  WHERE limit_id = ?1;";

const char *const sqlSelectLimitIds = "SELECT limit_id FROM speed_limit"
                                      "  WHERE limit_id < ?1 ORDER BY limit_id;";

const char *const sqlDeleteSpeedLimit = "DELETE FROM speed_limit WHERE limit_id = ?1"
                                        "  RETURNING inbound;";

const char *const sqlDeleteAppInSpeedLimit = "UPDATE app"
                                             "  SET in_limit_id = NULL"
                                             "  WHERE in_limit_id = ?1;";

const char *const sqlDeleteAppOutSpeedLimit = "UPDATE app"
                                              "  SET out_limit_id = NULL"
                                              "  WHERE out_limit_id = ?1;";

const char *const sqlUpdateSpeedLimitName = "UPDATE speed_limit SET name = ?2 WHERE limit_id = ?1;";

const char *const sqlUpdateSpeedLimitEnabled =
        "UPDATE speed_limit SET enabled = ?2 WHERE limit_id = ?1;";

bool driverWriteSpeedLimits(ConfBuffer &confBuf, bool onlyFlags = false)
{
    if (confBuf.hasError()) {
        qCWarning(LC) << "Driver config error:" << confBuf.errorMessage();
        return false;
    }

    auto driverManager = IoC<DriverManager>();
    if (!driverManager->writeSpeedLimits(confBuf.buffer(), onlyFlags)) {
        qCWarning(LC) << "Update driver error:" << driverManager->errorMessage();
        return false;
    }

    return true;
}

}

ConfSpeedLimitManager::ConfSpeedLimitManager(QObject *parent) : ConfManagerBase(parent) { }

bool ConfSpeedLimitManager::addOrUpdateSpeedLimit(SpeedLimit &limit)
{
    bool ok = true;

    beginWriteTransaction();

    const bool isNew = (limit.limitId == 0);
    if (isNew) {
        limit.limitId = DbQuery(sqliteDb(), &ok)
                                .sql(sqlSelectLimitIds)
                                .vars({ ConfUtil::speedLimitMaxCount() })
                                .getFreeId(/*maxId=*/ConfUtil::speedLimitMaxCount() - 1);
    }

    if (ok) {
        const QVariantList vars = {
            limit.limitId,
            limit.enabled,
            limit.inbound,
            limit.name,
            limit.kbps,
            limit.packetLoss,
            limit.latency,
            limit.bufferSize,
            DateUtil::now(),
        };

        DbQuery(sqliteDb(), &ok)
                .sql(isNew ? sqlInsertSpeedLimit : sqlUpdateSpeedLimit)
                .vars(vars)
                .executeOk();
    }

    endTransaction(ok);

    if (!ok)
        return false;

    if (isNew) {
        emit speedLimitAdded();
    } else {
        emit speedLimitUpdated(limit.limitId);
    }

    return true;
}

bool ConfSpeedLimitManager::deleteSpeedLimit(int limitId)
{
    // TODO
    return false;
}

bool ConfSpeedLimitManager::updateSpeedLimitName(int limitId, const QString &name)
{
    // TODO
    return false;
}

bool ConfSpeedLimitManager::updateSpeedLimitEnabled(int limitId, bool enabled)
{
    // TODO
    return false;
}

bool ConfSpeedLimitManager::updateDriverSpeedLimitFlag(int limitId, bool enabled)
{
    // TODO
    return false;
}
