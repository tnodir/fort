#include "confspeedlimitmanager.h"

#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/speedlimit.h>
#include <driver/drivermanager.h>
#include <fortglobal.h>
#include <util/conf/confbuffer.h>
#include <util/conf/confutil.h>
#include <util/dateutil.h>
#include <util/ioc/ioccontainer.h>

#include "confmanager.h"

namespace {

const QLoggingCategory LC("confSpeedLimit");

#define SELECT_SPEED_LIMIT_FIELDS                                                                  \
    "    t.limit_id,"                                                                              \
    "    t.enabled,"                                                                               \
    "    t.inbound,"                                                                               \
    "    t.packet_loss,"                                                                           \
    "    t.latency,"                                                                               \
    "    t.kbps,"                                                                                  \
    "    t.bufsize,"                                                                               \
    "    t.name,"                                                                                  \
    "    t.notes,"                                                                                 \
    "    t.mod_time"

const char *const sqlSelectSpeedLimits = "SELECT" SELECT_SPEED_LIMIT_FIELDS "  FROM speed_limit t"
                                         "  ORDER BY t.limit_id;";

const char *const sqlInsertSpeedLimit = "INSERT INTO speed_limit(limit_id, name, notes, enabled,"
                                        "    inbound, packet_loss, latency, kbps, bufsize,"
                                        "    mod_time)"
                                        "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10);";

const char *const sqlUpdateSpeedLimit = "UPDATE speed_limit"
                                        "  SET name = ?2, notes = ?3, enabled = ?4, inbound = ?5,"
                                        "    packet_loss = ?6, latency = ?7, kbps = ?8,"
                                        "    bufsize = ?9, mod_time = ?10"
                                        "  WHERE limit_id = ?1;";

const char *const sqlSelectSpeedLimitNameById = "SELECT name FROM speed_limit WHERE limit_id = ?1;";

const char *const sqlSelectSpeedLimitIds = "SELECT limit_id FROM speed_limit"
                                           "  WHERE limit_id <= ?1 ORDER BY limit_id;";

const char *const sqlDeleteSpeedLimit = "DELETE FROM speed_limit WHERE limit_id = ?1;";

const char *const sqlDeleteAppInSpeedLimit = "UPDATE app SET in_limit_id = NULL"
                                             "  WHERE in_limit_id = ?1;";

const char *const sqlDeleteAppOutSpeedLimit = "UPDATE app SET out_limit_id = NULL"
                                              "  WHERE out_limit_id = ?1;";

const char *const sqlUpdateSpeedLimitName = "UPDATE speed_limit SET name = ?2 WHERE limit_id = ?1;";

const char *const sqlUpdateSpeedLimitEnabled =
        "UPDATE speed_limit SET enabled = ?2 WHERE limit_id = ?1;";

const char *const sqlSelectEnabledSpeedLimitIds = "SELECT limit_id FROM speed_limit"
                                                  "  WHERE enabled = 1;";

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

ConfSpeedLimitManager::ConfSpeedLimitManager(QObject *parent) : ConfManagerBase(parent)
{
    setupSpeedLimitNamesCache();
}

void ConfSpeedLimitManager::setUp()
{
    auto confManager = Fort::dependency<ConfManager>();

    // The imported DB may reuse the ids
    connect(confManager, &ConfManager::imported, this,
            &ConfSpeedLimitManager::clearSpeedLimitNamesCache);
}

QString ConfSpeedLimitManager::speedLimitNameById(quint8 limitId)
{
    if (limitId == 0)
        return {};

    QString name = m_speedLimitNamesCache.value(limitId);

    if (name.isEmpty()) {
        name = DbQuery(sqliteDb())
                       .sql(sqlSelectSpeedLimitNameById)
                       .vars({ limitId })
                       .execute()
                       .toString();

        m_speedLimitNamesCache.insert(limitId, name);
    }

    return name;
}

bool ConfSpeedLimitManager::addOrUpdateSpeedLimit(SpeedLimit &limit)
{
    bool ok = true;

    beginWriteTransaction();

    const bool isNew = (limit.limitId == 0);
    if (isNew) {
        limit.limitId = DbQuery(sqliteDb(), &ok)
                                .sql(sqlSelectSpeedLimitIds)
                                .vars({ ConfUtil::speedLimitMaxCount() })
                                .getFreeId(/*maxId=*/ConfUtil::speedLimitMaxCount());
    }

    if (ok) {
        const QVariantList vars = {
            limit.limitId,
            limit.name,
            limit.notes,
            limit.enabled,
            limit.inbound,
            limit.packetLoss,
            limit.latency,
            limit.kbps,
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
        emit speedLimitUpdated();
    }

    updateDriverSpeedLimits();

    return true;
}

bool ConfSpeedLimitManager::deleteSpeedLimit(quint8 limitId)
{
    bool ok = false;

    beginWriteTransaction();

    const QVariantList vars = { limitId };

    if (DbQuery(sqliteDb(), &ok).sql(sqlDeleteSpeedLimit).vars(vars).executeOk()) {
        // Delete the Speed Limit from Programs
        DbQuery(sqliteDb(), &ok).sql(sqlDeleteAppInSpeedLimit).vars(vars).executeOk();
        DbQuery(sqliteDb(), &ok).sql(sqlDeleteAppOutSpeedLimit).vars(vars).executeOk();
    }

    endTransaction(ok);

    if (ok) {
        emit speedLimitRemoved(limitId);

        updateDriverSpeedLimits();
    }

    return ok;
}

bool ConfSpeedLimitManager::updateSpeedLimitName(quint8 limitId, const QString &name)
{
    bool ok = false;

    beginWriteTransaction();

    const QVariantList vars = { limitId, name };

    DbQuery(sqliteDb(), &ok).sql(sqlUpdateSpeedLimitName).vars(vars).executeOk();

    endTransaction(ok);

    if (ok) {
        emit speedLimitUpdated();
    }

    return ok;
}

bool ConfSpeedLimitManager::updateSpeedLimitEnabled(quint8 limitId, bool enabled)
{
    bool ok = false;

    beginWriteTransaction();

    const QVariantList vars = { limitId, enabled };

    DbQuery(sqliteDb(), &ok).sql(sqlUpdateSpeedLimitEnabled).vars(vars).executeOk();

    endTransaction(ok);

    if (ok) {
        emit speedLimitUpdated();

        updateDriverSpeedLimitFlags();
    }

    return ok;
}

bool ConfSpeedLimitManager::walkSpeedLimits(
        const std::function<walkSpeedLimitsCallback> &func) const
{
    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sqlSelectSpeedLimits).prepare(stmt))
        return false;

    while (stmt.step() == SqliteStmt::StepRow) {
        SpeedLimit limit;
        fillSpeedLimit(limit, stmt);

        if (!func(limit))
            return false;
    }

    return true;
}

void ConfSpeedLimitManager::updateDriverSpeedLimits()
{
    ConfBuffer confBuf;

    confBuf.writeSpeedLimits(*this);

    driverWriteSpeedLimits(confBuf);
}

void ConfSpeedLimitManager::updateDriverSpeedLimitFlags()
{
    quint32 enabledMask = 0;

    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sqlSelectEnabledSpeedLimitIds).prepare(stmt))
        return;

    while (stmt.step() == SqliteStmt::StepRow) {
        const int limitId = stmt.columnInt(0);

        if (limitId > 0 && limitId <= ConfUtil::speedLimitMaxCount()) {
            enabledMask |= (quint32(1) << (limitId - 1));
        }
    }

    ConfBuffer confBuf;

    confBuf.writeSpeedLimitFlags(enabledMask);

    driverWriteSpeedLimits(confBuf, /*onlyFlags=*/true);
}

void ConfSpeedLimitManager::setupSpeedLimitNamesCache()
{
    connect(this, &ConfSpeedLimitManager::speedLimitRemoved, this,
            &ConfSpeedLimitManager::clearSpeedLimitNamesCache);
    connect(this, &ConfSpeedLimitManager::speedLimitUpdated, this,
            &ConfSpeedLimitManager::clearSpeedLimitNamesCache);
}

void ConfSpeedLimitManager::clearSpeedLimitNamesCache()
{
    m_speedLimitNamesCache.clear();
}

void ConfSpeedLimitManager::fillSpeedLimit(SpeedLimit &limit, const SqliteStmt &stmt)
{
    limit.limitId = stmt.columnInt(0);
    limit.enabled = stmt.columnBool(1);
    limit.inbound = stmt.columnBool(2);
    limit.packetLoss = stmt.columnInt(3);
    limit.latency = stmt.columnUInt(4);
    limit.kbps = stmt.columnUInt(5);
    limit.bufferSize = stmt.columnUInt(6);
    limit.name = stmt.columnText(7);
    limit.notes = stmt.columnText(8);
    limit.modTime = stmt.columnDateTime(9);
}
