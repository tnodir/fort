#include "confmigration.h"

#include <QHash>
#include <QLoggingCategory>
#include <QSet>

#include <sqlite/dbquery.h>
#include <sqlite/dbvar.h>
#include <sqlite/sqlitestmt.h>

#include <fortglobal.h>
#include <fortsettings.h>
#include <qtcompat.h>
#include <util/conf/confutil.h>
#include <util/dateutil.h>
#include <util/fileutil.h>

#include "timeperiod.h"

using namespace Fort;

namespace {

const QLoggingCategory LC("conf");

inline constexpr int DATABASE_USER_VERSION = 72;

using AppsMap = QHash<qint64, QString>;
using AppIdsArray = QVector<qint64>;

bool fillAppPathsMap(SqliteDb *db, AppsMap &appsMap)
{
    const char *const sql = "SELECT app_id, origin_path, path FROM app;";

    SqliteStmt stmt;
    if (!DbQuery(db).sql(sql).prepare(stmt))
        return false;

    while (stmt.step() == SqliteStmt::StepRow) {
        const qint64 appId = stmt.columnInt64(0);
        const QString appOriginPath = stmt.columnText(1);
        const QString appPath = stmt.columnText(2);

        appsMap.insert(appId, !appOriginPath.isEmpty() ? appOriginPath : appPath);
    }

    return true;
}

bool updateAppPathsByMap(SqliteDb *db, const AppsMap &appsMap, AppIdsArray &dupAppIds)
{
    const char *const sqlUpdateAppPaths = "UPDATE app"
                                          "  SET path = ?2, origin_path = ?3"
                                          "  WHERE app_id = ?1;";

    SqliteStmt stmt;
    if (!DbQuery(db).sql(sqlUpdateAppPaths).prepare(stmt))
        return false;

    QSet<QString> appPaths;

    for (const auto &[appId, appOriginPath] : QtCompat::keyValueRange(appsMap)) {
        const QString appPath = FileUtil::normalizePath(appOriginPath);

        if (Q_LIKELY(!appPaths.contains(appPath))) {
            appPaths.insert(appPath);
        } else {
            // Duplicate path found
            dupAppIds.append(appId);
            continue;
        }

        stmt.bindInt64(1, appId);
        stmt.bindText(2, appPath);
        stmt.bindText(3, appOriginPath);

        if (stmt.step() != SqliteStmt::StepDone)
            return false;

        stmt.reset();
    }

    return true;
}

void deleteDupApps(SqliteDb *db, const AppIdsArray &dupAppIds)
{
    if (dupAppIds.isEmpty())
        return;

    const char *const sqlDeleteApp = "DELETE FROM app WHERE path = ?1;";

    for (qint64 appId : dupAppIds) {
        qCDebug(LC) << "Migrate: Remove dup app-id:" << appId;
        DbQuery(db).sql(sqlDeleteApp).vars({ appId }).executeOk();
    }

    // Remove alerts for deleted apps
    db->execute("DELETE FROM app_alert WHERE NOT EXISTS ("
                "  SELECT 1 FROM app WHERE app.app_id = app_alert.app_id);");
}

bool migrateAppPaths(SqliteDb *db)
{
    AppsMap appsMap;
    AppIdsArray dupAppIds;

    if (!(fillAppPathsMap(db, appsMap) && updateAppPathsByMap(db, appsMap, dupAppIds)))
        return false;

    deleteDupApps(db, dupAppIds);

    return true;
}

using AddrGroupTextsMap = QHash<qint64, QString>;

bool fillAddrGroupExcludeTextsMap(
        SqliteDb *db, AddrGroupTextsMap &textsMap, const QString &removedLine)
{
    const char *const sql = "SELECT addr_group_id, exclude_text FROM address_group;";

    SqliteStmt stmt;
    if (!DbQuery(db).sql(sql).prepare(stmt))
        return false;

    while (stmt.step() == SqliteStmt::StepRow) {
        const qint64 addrGroupId = stmt.columnInt64(0);
        QStringList lines = stmt.columnText(1).split('\n');

        const auto removedCount =
                lines.removeIf([&](const QString &line) { return line.trimmed() == removedLine; });

        if (removedCount > 0) {
            textsMap.insert(addrGroupId, lines.join('\n'));
        }
    }

    return true;
}

// The old local networks list had the removed line
bool migrateAddrGroupExcludeTexts(SqliteDb *db, const QString &removedLine)
{
    AddrGroupTextsMap textsMap;

    if (!fillAddrGroupExcludeTextsMap(db, textsMap, removedLine))
        return false;

    const char *const sql = "UPDATE address_group SET exclude_text = ?2 WHERE addr_group_id = ?1;";

    for (const auto &[addrGroupId, excludeText] : QtCompat::keyValueRange(textsMap)) {
        if (!DbQuery(db).sql(sql).vars({ addrGroupId, excludeText }).executeOk())
            return false;
    }

    return true;
}

// The old "Allowed Internet Addresses" kept the hidden "Include" list: it's "Allow Addresses" now
bool migrateAddrGroupAllowInclude(SqliteDb *db)
{
    const char *const sql = "UPDATE address_group"
                            "  SET include_all = 1, include_zones = 0, include_text = ''"
                            "  WHERE addr_group_id = (SELECT addr_group_id FROM address_group"
                            "    ORDER BY addr_group_id LIMIT 1 OFFSET 1);";

    return DbQuery(db).sql(sql).executeOk();
}

struct OldAppGroup
{
    bool enabled : 1 = false;
    bool periodEnabled : 1 = false;
    bool limitInEnabled : 1 = false;
    bool limitOutEnabled : 1 = false;

    quint8 periodId = 0; // migrated Time Period

    quint16 limitPacketLoss = 0;

    quint32 limitLatency = 0;
    quint32 speedLimitIn = 0;
    quint32 speedLimitOut = 0;
    quint32 limitBufferSizeIn = 0;
    quint32 limitBufferSizeOut = 0;

    qint64 appGroupId = 0;

    QString name;
    QString periodFrom;
    QString periodTo;
};

QString oldEntityName(const QString &tableName)
{
    return SqliteDb::entityName(SqliteDb::migrationOldSchemaName(), tableName);
}

bool loadOldAppGroups(SqliteDb *db, quint32 appGroupBits, QList<OldAppGroup> &appGroups)
{
    // Skip the default "Main" App. Group: it has all the Programs
    const QString sql = "SELECT app_group_id, order_index, period_enabled,"
                        "    limit_in_enabled, limit_out_enabled,"
                        "    limit_packet_loss, limit_latency,"
                        "    speed_limit_in, speed_limit_out,"
                        "    limit_bufsize_in, limit_bufsize_out,"
                        "    name, period_from, period_to"
                        "  FROM "
            + oldEntityName("app_group") + "  WHERE order_index > 0 ORDER BY order_index;";

    SqliteStmt stmt;
    if (!DbQuery(db).sql(sql).prepare(stmt))
        return false; // there are no App. Groups

    while (stmt.step() == SqliteStmt::StepRow) {
        OldAppGroup appGroup;
        appGroup.appGroupId = stmt.columnInt64(0);
        // The App. Group's enabled flag is kept in the .ini by its index
        const int orderIndex = stmt.columnInt(1);
        appGroup.enabled = (orderIndex < 32 && (appGroupBits & (1u << orderIndex)) != 0);
        appGroup.periodEnabled = stmt.columnBool(2);
        appGroup.limitInEnabled = stmt.columnBool(3);
        appGroup.limitOutEnabled = stmt.columnBool(4);
        appGroup.limitPacketLoss = stmt.columnInt(5);
        appGroup.limitLatency = stmt.columnUInt(6);
        appGroup.speedLimitIn = stmt.columnUInt(7);
        appGroup.speedLimitOut = stmt.columnUInt(8);
        appGroup.limitBufferSizeIn = stmt.columnUInt(9);
        appGroup.limitBufferSizeOut = stmt.columnUInt(10);
        appGroup.name = stmt.columnText(11);
        appGroup.periodFrom = stmt.columnText(12);
        appGroup.periodTo = stmt.columnText(13);

        appGroups.append(appGroup);
    }

    return true;
}

void clearTimePeriods(SqliteDb *db)
{
    db->execute("DELETE FROM time_period;");
    db->execute("DELETE FROM time_period_interval;");
}

// The Time Period with the same name (period) is reused
int migrateTimePeriod(SqliteDb *db, const QString &periodFrom, const QString &periodTo)
{
    const QString name = periodFrom + '-' + periodTo;

    const int existingPeriodId = DbQuery(db)
                                         .sql("SELECT period_id FROM time_period WHERE name = ?1;")
                                         .vars({ name })
                                         .execute()
                                         .toInt();
    if (existingPeriodId != 0)
        return existingPeriodId;

    bool ok = true;

    const int periodId = DbQuery(db, &ok)
                                 .sql("SELECT period_id FROM time_period"
                                      "  WHERE period_id <= ?1 ORDER BY period_id;")
                                 .vars({ ConfUtil::timePeriodMaxCount() })
                                 .getFreeId(/*maxId=*/ConfUtil::timePeriodMaxCount());
    if (!ok) {
        qCWarning(LC) << "Migrate: No free Time Period id for the period:" << name;
        return 0;
    }

    DbQuery(db, &ok)
            .sql("INSERT INTO time_period(period_id, name, mod_time) VALUES(?1, ?2, ?3);")
            .vars({ periodId, name, DateUtil::now() })
            .executeOk();

    // The old period of the equal times was never active, the new one is the whole 24 hours
    const quint8 weekDays = (periodFrom == periodTo) ? 0 : TimePeriodAllWeekDays;

    DbQuery(db, &ok)
            .sql("INSERT INTO time_period_interval(period_id, order_index, week_days,"
                 "    time_from, time_to)"
                 "  VALUES(?1, 0, ?2, ?3, ?4);")
            .vars({ periodId, weekDays, periodFrom, periodTo })
            .executeOk();

    return ok ? periodId : 0;
}

// The disabled period is kept, unless it's the default one
bool hasOldPeriod(bool periodEnabled, const QString &periodFrom, const QString &periodTo)
{
    return periodEnabled || periodFrom != periodTo;
}

QString oldAppGroupAppIdsSql()
{
    return "SELECT app_id FROM " + oldEntityName("app") + " WHERE app_group_id = ?1";
}

bool migrateOldAppGroup(SqliteDb *db, OldAppGroup &appGroup)
{
    bool ok = true;

    const int groupId = DbQuery(db, &ok)
                                .sql("SELECT group_id FROM app_group"
                                     "  WHERE group_id <= ?1 ORDER BY group_id;")
                                .vars({ ConfUtil::groupMaxCount() })
                                .getFreeId(/*maxId=*/ConfUtil::groupMaxCount());
    if (!ok) {
        qCWarning(LC) << "Migrate: No free Group id for the App. Group:" << appGroup.name;
        return false;
    }

    appGroup.periodId = hasOldPeriod(appGroup.periodEnabled, appGroup.periodFrom, appGroup.periodTo)
            ? migrateTimePeriod(db, appGroup.periodFrom, appGroup.periodTo)
            : 0;

    const QVariantList vars = {
        groupId,
        appGroup.enabled,
        appGroup.periodEnabled,
        appGroup.name,
        DbVar::nullable(appGroup.periodId),
        DateUtil::now(),
    };

    DbQuery(db, &ok)
            .sql("INSERT INTO app_group(group_id, enabled, exclusive, period_enabled, name,"
                 "    period_id, mod_time)"
                 "  VALUES(?1, ?2, 0, ?3, ?4, ?5, ?6);")
            .vars(vars)
            .executeOk();
    if (!ok)
        return false;

    const quint32 groupBit = (quint32(1) << (groupId - 1));

    DbQuery(db, &ok)
            .sql("UPDATE app SET groups_mask = groups_mask | ?2"
                 "  WHERE app_id IN ("
                    + oldAppGroupAppIdsSql() + ");")
            .vars({ appGroup.appGroupId, groupBit })
            .executeOk();

    return ok;
}

bool migrateOldAppGroupAppsSpeedLimit(
        SqliteDb *db, const OldAppGroup &appGroup, int limitId, bool inbound)
{
    const QString sql =
            QString("UPDATE app SET %1 = ?2 WHERE app_id IN (%2);")
                    .arg(inbound ? "in_limit_id" : "out_limit_id", oldAppGroupAppIdsSql());

    bool ok = true;

    DbQuery(db, &ok).sql(sql).vars({ appGroup.appGroupId, limitId }).executeOk();

    return ok;
}

bool migrateOldAppGroupSpeedLimit(SqliteDb *db, const OldAppGroup &appGroup, bool inbound)
{
    const quint32 kbps = inbound ? appGroup.speedLimitIn : appGroup.speedLimitOut;
    // The disabled App. Group didn't limit its Programs
    const bool limitEnabled =
            appGroup.enabled && (inbound ? appGroup.limitInEnabled : appGroup.limitOutEnabled);

    if (kbps == 0)
        return true;

    bool ok = true;

    const int limitId = DbQuery(db, &ok)
                                .sql("SELECT limit_id FROM speed_limit"
                                     "  WHERE limit_id <= ?1 ORDER BY limit_id;")
                                .vars({ ConfUtil::speedLimitMaxCount() })
                                .getFreeId(/*maxId=*/ConfUtil::speedLimitMaxCount());
    if (!ok) {
        qCWarning(LC) << "Migrate: No free Speed Limit id for the App. Group:" << appGroup.name;
        return false;
    }

    const QVariantList vars = {
        limitId,
        limitEnabled,
        inbound,
        appGroup.periodEnabled, // nor did it limit out of its period
        appGroup.name,
        DbVar::nullable(appGroup.periodId),
        appGroup.limitPacketLoss,
        appGroup.limitLatency,
        kbps,
        inbound ? appGroup.limitBufferSizeIn : appGroup.limitBufferSizeOut,
        DateUtil::now(),
    };

    DbQuery(db, &ok)
            .sql("INSERT INTO speed_limit(limit_id, enabled, inbound, period_enabled, name,"
                 "    period_id, packet_loss, latency, kbps, bufsize, mod_time)"
                 "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11);")
            .vars(vars)
            .executeOk();
    if (!ok)
        return false;

    return migrateOldAppGroupAppsSpeedLimit(db, appGroup, limitId, inbound);
}

// The App. Groups are replaced by the Groups and Speed Limits
void migrateAppGroups(SqliteDb *db, quint32 appGroupBits)
{
    // The old App. Groups are copied to the Groups' table with the same name
    db->execute("DELETE FROM app_group;");

    // The old App. Groups' speed limits replace the current Speed Limits on import
    db->execute("DELETE FROM speed_limit;");

    // The old App. Groups' periods replace the current Time Periods on import
    clearTimePeriods(db);

    QList<OldAppGroup> appGroups;

    if (!loadOldAppGroups(db, appGroupBits, appGroups))
        return;

    for (OldAppGroup &appGroup : appGroups) {
        qCDebug(LC) << "Migrate: App. Group:" << appGroup.name;

        if (!migrateOldAppGroup(db, appGroup))
            continue;

        migrateOldAppGroupSpeedLimit(db, appGroup, /*inbound=*/true);
        migrateOldAppGroupSpeedLimit(db, appGroup, /*inbound=*/false);
    }
}

// The Groups' periods are replaced by the Time Periods
bool migrateGroupPeriods(SqliteDb *db)
{
    // The old Groups' periods replace the current Time Periods on import
    clearTimePeriods(db);

    // The "period_enabled" column is copied as is
    const QString sql = "SELECT group_id, period_enabled, period_from, period_to FROM "
            + oldEntityName("app_group");

    SqliteStmt stmt;
    if (!DbQuery(db).sql(sql).prepare(stmt))
        return false;

    while (stmt.step() == SqliteStmt::StepRow) {
        const qint64 groupId = stmt.columnInt64(0);
        const QString periodFrom = stmt.columnText(2);
        const QString periodTo = stmt.columnText(3);

        if (!hasOldPeriod(stmt.columnBool(1), periodFrom, periodTo))
            continue;

        const int periodId = migrateTimePeriod(db, periodFrom, periodTo);

        DbQuery(db)
                .sql("UPDATE app_group SET period_id = ?2 WHERE group_id = ?1;")
                .vars({ groupId, DbVar::nullable(periodId) })
                .executeOk();
    }

    return true;
}

// The Tasks' intervals are in minutes instead of hours
bool migrateTaskIntervals(SqliteDb *db)
{
    const QString sql = "UPDATE task SET interval_minutes ="
                        "  (SELECT t.interval_hours * 60 FROM "
            + oldEntityName("task") + " t WHERE t.task_id = task.task_id);";

    return DbQuery(db).sql(sql).executeOk();
}

// The Statistics' active period is replaced by the Time Period
void migrateStatActivePeriod(SqliteDb *db)
{
    auto settings = Fort::settings();

    const QString periodFromText = settings->activePeriodFrom();
    const QString periodToText = settings->activePeriodTo();
    if (periodFromText.isEmpty() && periodToText.isEmpty())
        return; // no old active period

    const QString periodFrom = DateUtil::reformatTime(periodFromText);
    const QString periodTo = DateUtil::reformatTime(periodToText);

    const int periodId = hasOldPeriod(settings->activePeriodEnabled(), periodFrom, periodTo)
            ? migrateTimePeriod(db, periodFrom, periodTo)
            : 0;

    settings->setActivePeriodId(periodId);
}

void migrateTimePeriods(SqliteDb *db, int version)
{
    // COMPAT: Migrate the App. Groups to the Groups and Speed Limits
    if (version < 59) {
        migrateAppGroups(db, settings()->appGroupBits());
    }

    // COMPAT: Migrate the Groups' periods to the Time Periods
    if (version >= 59 && version < 60) {
        migrateGroupPeriods(db);
    }

    // COMPAT: Migrate the Statistics' active period to the Time Period
    if (version < 66) {
        migrateStatActivePeriod(db);
    }
}

// The Program's Rule is the first one of its Rules
bool migrateAppRules(SqliteDb *db)
{
    const QString sql = "INSERT INTO app_rule(app_id, rule_id, order_index)"
                        "  SELECT t.app_id, t.rule_id, 1 FROM "
            + oldEntityName("app")
            + " t"
              "    JOIN app a ON a.app_id = t.app_id" // the migrated ones
              "    WHERE t.rule_id IS NOT NULL;";

    return DbQuery(db).sql(sql).executeOk();
}

// The Programs' filters are separated by the ";" line: a filter may be of several lines
bool migrateAppFiltersTexts(SqliteDb *db)
{
    return DbQuery(db)
            .sql("UPDATE app SET filters_text = replace(filters_text, char(10), char(10, 59, 10))"
                 "  WHERE filters_text IS NOT NULL;")
            .executeOk();
}

void migrateApps(SqliteDb *db, int version)
{
    // COMPAT: The Program's Rule is in its Rules
    if (version < 71) {
        migrateAppRules(db);
    }

    // COMPAT: The Program's filters of several lines
    if (version < 72) {
        migrateAppFiltersTexts(db);
    }
}

bool migrateFunc(SqliteDb *db, int version, bool isNewDb, void *ctx)
{
    Q_UNUSED(ctx);

    if (isNewDb) {
        // COMPAT: DB schema
        return true;
    }

    // COMPAT: DB content
    if (version < 21) {
        migrateAppPaths(db);
    }

    // COMPAT: Remove "::/0" from addr group's exclude: the whole IPv6 address space
    if (version < 58) {
        migrateAddrGroupExcludeTexts(db, "::/0");
    }

    // COMPAT: Migrate the App. Groups and the old periods to the Time Periods
    migrateTimePeriods(db, version);

    // COMPAT: Migrate the Tasks' intervals from hours to minutes
    if (version < 61) {
        migrateTaskIntervals(db);
    }

    // COMPAT: Clear the old "Include" list of the allowed addresses
    if (version < 68) {
        migrateAddrGroupAllowInclude(db);
    }

    // COMPAT: Remove "2001::/32" from addr group's exclude: the Teredo's routable prefix
    if (version < 69) {
        migrateAddrGroupExcludeTexts(db, "2001::/32");
    }

    // COMPAT: Migrate the Programs' Rules and filters
    migrateApps(db, version);

    return true;
}

}

SqliteDb::MigrateOptions ConfMigration::migrateOptions()
{
    SqliteDb::MigrateOptions opt = {
        .sqlDir = ":/conf/migrations",
        .version = DATABASE_USER_VERSION,
        .recreate = true,
        .migrateFunc = &migrateFunc,
        .ftsTables = {
            {
                .contentTable = "app",
                .contentRowid = "app_id",
                .columns = { "path", "name", "notes" },
            },
            {
                .contentTable = "rule",
                .contentRowid = "rule_id",
                .columns = { "name", "notes" },
            },
        },
    };

    return opt;
}
