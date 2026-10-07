#include "confmanager.h"

#include <QHash>
#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/dbvar.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <driver/drivercommon.h>
#include <driver/drivermanager.h>
#include <fortglobal.h>
#include <fortsettings.h>
#include <manager/envmanager.h>
#include <manager/serviceinfomanager.h>
#include <manager/windowmanager.h>
#include <task/taskinfo.h>
#include <task/taskinfozonedownloader.h>
#include <task/taskmanager.h>
#include <user/usersettings.h>
#include <util/conf/confbuffer.h>
#include <util/conf/confutil.h>
#include <util/dateutil.h>
#include <util/fileutil.h>

#include "addressgroup.h"
#include "confappmanager.h"
#include "confgroupmanager.h"
#include "confrulemanager.h"
#include "conftimeperiodmanager.h"
#include "filtersimconn.h"
#include "timeperiod.h"

using namespace Fort;

namespace {

const QLoggingCategory LC("conf");

inline constexpr int DATABASE_USER_VERSION = 63;

const char *const sqlSelectAddressGroups = "SELECT addr_group_id, include_all, exclude_all,"
                                           "    include_zones, exclude_zones,"
                                           "    include_text, exclude_text"
                                           "  FROM address_group"
                                           "  ORDER BY addr_group_id;";

const char *const sqlInsertAddressGroup = "INSERT INTO address_group(addr_group_id,"
                                          "    include_all, exclude_all,"
                                          "    include_zones, exclude_zones,"
                                          "    include_text, exclude_text)"
                                          "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7);";

const char *const sqlUpdateAddressGroup = "UPDATE address_group"
                                          "  SET include_all = ?2, exclude_all = ?3,"
                                          "    include_zones = ?4, exclude_zones = ?5,"
                                          "    include_text = ?6, exclude_text = ?7"
                                          "  WHERE addr_group_id = ?1;";

const char *const sqlSelectTaskByName = "SELECT task_id, enabled,"
                                        "    run_on_startup, delay_startup,"
                                        "    max_retries, retry_seconds, interval_minutes,"
                                        "    last_run, last_success, data"
                                        "  FROM task"
                                        "  WHERE name = ?1;";

const char *const sqlInsertTask = "INSERT INTO task(task_id, name, enabled,"
                                  "    run_on_startup, delay_startup,"
                                  "    max_retries, retry_seconds, interval_minutes,"
                                  "    last_run, last_success, data)"
                                  "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11);";

const char *const sqlUpdateTask = "UPDATE task"
                                  "  SET name = ?2, enabled = ?3,"
                                  "    run_on_startup = ?4, delay_startup = ?5,"
                                  "    max_retries = ?6, retry_seconds = ?7, interval_minutes = ?8,"
                                  "    last_run = ?9, last_success = ?10,"
                                  "    data = ?11"
                                  "  WHERE task_id = ?1;";

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

    auto it = appsMap.begin();
    for (; it != appsMap.end(); ++it) {
        const qint64 appId = it.key();
        const QString appOriginPath = it.value();
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

bool fillAddrGroupExcludeTextsMap(SqliteDb *db, AddrGroupTextsMap &textsMap)
{
    const char *const sql = "SELECT addr_group_id, exclude_text FROM address_group;";

    SqliteStmt stmt;
    if (!DbQuery(db).sql(sql).prepare(stmt))
        return false;

    while (stmt.step() == SqliteStmt::StepRow) {
        const qint64 addrGroupId = stmt.columnInt64(0);
        QStringList lines = stmt.columnText(1).split('\n');

        const auto removedCount = lines.removeIf(
                [](const QString &line) { return line.trimmed() == QLatin1String("::/0"); });

        if (removedCount > 0) {
            textsMap.insert(addrGroupId, lines.join('\n'));
        }
    }

    return true;
}

// The old local networks list had "::/0", which is the whole IPv6 address space
bool migrateAddrGroupExcludeTexts(SqliteDb *db)
{
    AddrGroupTextsMap textsMap;

    if (!fillAddrGroupExcludeTextsMap(db, textsMap))
        return false;

    const char *const sql = "UPDATE address_group SET exclude_text = ?2 WHERE addr_group_id = ?1;";

    auto it = textsMap.constBegin();
    for (; it != textsMap.constEnd(); ++it) {
        if (!DbQuery(db).sql(sql).vars({ it.key(), it.value() }).executeOk())
            return false;
    }

    return true;
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

    // COMPAT: Remove "::/0" from addr group's exclude
    if (version < 58) {
        migrateAddrGroupExcludeTexts(db);
    }

    // COMPAT: Migrate the App. Groups to the Groups and Speed Limits
    if (version < 59) {
        migrateAppGroups(db, settings()->appGroupBits());
    }

    // COMPAT: Migrate the Groups' periods to the Time Periods
    if (version >= 59 && version < 60) {
        migrateGroupPeriods(db);
    }

    // COMPAT: Migrate the Tasks' intervals from hours to minutes
    if (version < 61) {
        migrateTaskIntervals(db);
    }

    return true;
}

SqliteDb::MigrateOptions migrateOptions()
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

bool loadAddressGroups(SqliteDb *db, const QList<AddressGroup *> &addressGroups, int &index)
{
    SqliteStmt stmt;
    if (!DbQuery(db).sql(sqlSelectAddressGroups).prepare(stmt))
        return false;

    index = 0;
    while (stmt.step() == SqliteStmt::StepRow) {
        auto addrGroup = addressGroups.at(index);
        Q_ASSERT(addrGroup);

        addrGroup->setId(stmt.columnInt64(0));
        addrGroup->setIncludeAll(stmt.columnBool(1));
        addrGroup->setExcludeAll(stmt.columnBool(2));
        addrGroup->setIncludeZones(stmt.columnUInt(3));
        addrGroup->setExcludeZones(stmt.columnUInt(4));
        addrGroup->setIncludeText(stmt.columnText(5));
        addrGroup->setExcludeText(stmt.columnText(6));

        addrGroup->setEdited(false);

        if (++index > 1)
            break;
    }

    return true;
}

bool saveAddressGroup(SqliteDb *db, AddressGroup *addrGroup)
{
    const bool rowExists = (addrGroup->id() != 0);
    if (!addrGroup->edited() && rowExists)
        return true;

    const QVariantList vars = {
        DbVar::nullable(addrGroup->id(), !rowExists),
        addrGroup->includeAll(),
        addrGroup->excludeAll(),
        qint64(addrGroup->includeZones()),
        qint64(addrGroup->excludeZones()),
        addrGroup->includeText(),
        addrGroup->excludeText(),
    };

    const char *sql = rowExists ? sqlUpdateAddressGroup : sqlInsertAddressGroup;

    if (!DbQuery(db).sql(sql).vars(vars).executeOk())
        return false;

    if (!rowExists) {
        addrGroup->setId(db->lastInsertRowid());
    }

    addrGroup->setEdited(false);

    return true;
}

bool saveAddressGroups(SqliteDb *db, const FirewallConf &conf)
{
    for (AddressGroup *addrGroup : conf.addressGroups()) {
        if (!saveAddressGroup(db, addrGroup))
            return false;
    }
    return true;
}

bool exportFile(const QString &filePath, const QString &path)
{
    const QString fileName = FileUtil::fileName(filePath);
    const QString destFilePath = path + fileName;

    if (!FileUtil::replaceFile(filePath, destFilePath)) {
        qCWarning(LC) << "Export file error from:" << filePath << "to:" << destFilePath;
        return false;
    }

    return true;
}

bool importFile(const QString &filePath, const QString &path)
{
    const QString fileName = FileUtil::fileName(filePath);
    const QString srcFilePath = path + fileName;

    if (!FileUtil::fileExists(srcFilePath))
        return true;

    if (!FileUtil::replaceFile(srcFilePath, filePath)) {
        qCWarning(LC) << "Import file error from:" << srcFilePath << "to:" << filePath;
        return false;
    }

    FileUtil::resetFilePermissions(filePath);

    return true;
}

void showErrorMessage(const QString &errorMessage)
{
    windowManager()->showErrorBox(errorMessage);
}

}

ConfManager::ConfManager(const QString &filePath, QObject *parent, quint32 openFlags) :
    QObject(parent), m_sqliteDb(new SqliteDb(filePath, openFlags))
{
    setupTimers();
}

void ConfManager::setUp()
{
    setupDb();
}

void ConfManager::applyFilterOffSeconds()
{
    const bool isFilterOff = !conf().filterEnabled();
    const int filterOffMsec = isFilterOff ? ini().filterOffSeconds() * 1000 : 0;

    const bool isTimerActive =
            (m_filterOffTimer.isActive() && m_filterOffTimer.interval() == filterOffMsec);
    if (isTimerActive)
        return;

    m_filterOffTimer.stop();

    if (filterOffMsec > 0) {
        m_filterOffTimer.start(filterOffMsec);
    }
}

void ConfManager::applyAutoLearnSeconds()
{
    const bool isAutoLearn = (conf().filterMode() == FirewallConf::ModeAutoLearn);
    const int autoLearnMsec = isAutoLearn ? ini().autoLearnSeconds() * 1000 : 0;

    const bool isTimerActive =
            (m_autoLearnTimer.isActive() && m_autoLearnTimer.interval() == autoLearnMsec);
    if (isTimerActive)
        return;

    m_autoLearnTimer.stop();

    if (autoLearnMsec > 0) {
        m_autoLearnTimer.start(autoLearnMsec);
    }
}

void ConfManager::setupTimers()
{
    m_filterOffTimer.setSingleShot(true);
    connect(&m_filterOffTimer, &QTimer::timeout, this, &ConfManager::switchFilterOff);

    m_autoLearnTimer.setSingleShot(true);
    connect(&m_autoLearnTimer, &QTimer::timeout, this, &ConfManager::switchAutoLearn);
}

void ConfManager::switchFilterOff()
{
    conf().setFilterEnabled(true);

    saveFlags();
}

void ConfManager::switchAutoLearn()
{
    conf().setFilterMode(FirewallConf::ModeBlockAll);

    saveFlags();
}

bool ConfManager::setupDb()
{
    if (!sqliteDb()->open()) {
        qCCritical(LC) << "File open error:" << sqliteDb()->filePath()
                       << sqliteDb()->errorMessage();
        return false;
    }

    SqliteDb::MigrateOptions opt = migrateOptions();

    if (!sqliteDb()->migrate(opt)) {
        qCCritical(LC) << "Migration error" << sqliteDb()->filePath();
        return false;
    }

    return true;
}

void ConfManager::setupDefault(FirewallConf &conf) const
{
    conf.setupDefaultAddressGroups();
}

bool ConfManager::checkCanMigrate(Settings *settings) const
{
    QString viaVersion;
    if (!settings->canMigrate(viaVersion)) {
        showErrorMessage(tr("Please first install Fort Firewall v%1 and save Options from it.")
                        .arg(viaVersion));
        return false;
    }
    return true;
}

bool ConfManager::loadConf(FirewallConf &conf)
{
    if (conf.optEdited()) {
        bool isNewConf = false;

        if (!loadFromDb(conf, isNewConf))
            return false;

        if (isNewConf) {
            setupDefault(conf);
            saveToDb(conf);
        }
    }

    settings()->readConfIni(conf);

    return true;
}

void ConfManager::load()
{
    if (!loadConf(conf())) {
        showErrorMessage(tr("Cannot load Settings"));
        return;
    }

    applySavedConf(conf());
}

bool ConfManager::save(FirewallConf &newConf, IniOptions &ini)
{
    if (!newConf.anyEdited())
        return true;

    if (!(validateConf(newConf) && saveConf(newConf, ini)))
        return false;

    applySavedConf(newConf);

    return true;
}

bool ConfManager::saveConf(FirewallConf &conf, IniOptions &ini)
{
    qCDebug(LC) << "Conf save";

    if (conf.optEdited() && !saveToDb(conf))
        return false;

    settings()->writeConfIni(conf, ini);

    // Tasks
    if (conf.taskEdited()) {
        saveTasksByIni(ini);
    }

    ini.clear();

    return true;
}

void ConfManager::applySavedConf(FirewallConf &newConf)
{
    if (!newConf.anyEdited())
        return;

    const bool onlyFlags = !newConf.optEdited();

    if (&conf() != &newConf) {
        if (onlyFlags) {
            conf().copyFlags(newConf);
            newConf.resetEdited();
        } else {
            conf().copy(newConf);
        }
    }

    applyFilterOffSeconds();
    applyAutoLearnSeconds();

    emit confChanged(onlyFlags, conf().editedFlags());

    if (conf().iniEdited()) {
        emit iniChanged();
    }

    conf().resetEdited();
}

bool ConfManager::saveFlags()
{
    conf().setFlagsEdited();

    return save(conf(), ini());
}

void ConfManager::saveIni()
{
    conf().setIniEdited();

    saveConf(conf(), ini());

    conf().resetEdited();
}

void ConfManager::saveIniUser()
{
    iniUser().saveAndClear();
}

void ConfManager::saveIniUser(IniUser &iniUser, bool onlyFlags)
{
    iniUser.saveAndClear();

    emit iniUserChanged(onlyFlags);
}

QVariant ConfManager::toPatchVariant(const IniOptions &ini, bool onlyFlags, uint editedFlags) const
{
    return onlyFlags ? conf().toVariant(ini, /*onlyEdited=*/true) // send only flags to clients
                     : FirewallConf::editedFlagsToVariant(
                               editedFlags); // clients have to reload all from storage
}

bool ConfManager::saveVariant(const QVariant &confVar)
{
    auto &ini = Fort::ini();

    conf().fromVariant(ini, confVar, /*onlyEdited=*/true);

    if (!saveConf(conf(), ini))
        return false;

    applySavedConf(conf());

    return true;
}

bool ConfManager::loadTasks(const QList<TaskInfo *> &taskInfos)
{
    for (TaskInfo *taskInfo : taskInfos) {
        if (!loadTask(taskInfo))
            return false;
    }

    return true;
}

bool ConfManager::saveTasks(const QList<TaskInfo *> &taskInfos)
{
    bool ok = true;

    beginWriteTransaction();

    for (TaskInfo *taskInfo : taskInfos) {
        if (!saveTask(taskInfo)) {
            ok = false;
            break;
        }
    }

    endTransaction(ok);

    return ok;
}

bool ConfManager::exportBackup(const QString &path)
{
    FileUtil::makePath(path);

    const QString outPath = FileUtil::pathSlash(path);

    // Save the windows' states to the User Ini
    windowManager()->saveAllWindowStates();

    // Export User Ini
    if (!exportFile(userSettings()->filePath(), outPath))
        return false;

    // Export DB
    if (!exportMasterBackup(outPath)) {
        qCWarning(LC) << "Export error:" << path;
        return false;
    }

    return true;
}

bool ConfManager::exportMasterBackup(const QString &path)
{
    // Export Ini
    {
        auto settings = Fort::settings();

        if (!exportFile(settings->filePath(), path))
            return false;
    }

    // Export Db
    if (!backupDbFile(path)) {
        qCWarning(LC) << "Export Db error:" << sqliteDb()->errorMessage();
        return false;
    }

    return true;
}

bool ConfManager::importBackup(const QString &path)
{
    const QString inPath = FileUtil::pathSlash(path);

    // Import User Ini
    {
        auto settings = Fort::userSettings();

        if (!checkCanMigrate(settings))
            return false;

        // The windows save their states to the User Ini on close
        windowManager()->closeAllWindows(/*isAppQuitting=*/true);

        if (!importFile(settings->filePath(), inPath))
            return false;

        settings->reload();

        emit iniUserChanged(/*onlyFlags=*/false);
    }

    // Import DB
    if (!importMasterBackup(inPath)) {
        qCWarning(LC) << "Import error:" << path;
        return false;
    }

    return true;
}

bool ConfManager::importMasterBackup(const QString &path)
{
    emit aboutToImport();

    // Import Ini
    {
        auto settings = Fort::settings();

        if (!importFile(settings->filePath(), path))
            return false;

        settings->reload();
    }

    // Import Db
    SqliteDb::MigrateOptions opt = migrateOptions();

    opt.backupFilePath = path + FileUtil::fileName(sqliteDb()->filePath());

    if (!sqliteDb()->import(opt))
        return false;

    emit imported();

    // The edited flags are reset after the first load()
    conf().resetEdited(FirewallConf::AllEdited);

    load(); // Reload conf

    return true;
}

bool ConfManager::checkPassword(const QString &password)
{
    return settings()->checkPassword(password);
}

bool ConfManager::simulateConn(FilterSimConn &simConn)
{
    // Write the buffers as for the driver
    ConfBuffer confBuf;
    if (!confBuf.writeConf(conf(), confAppManager(), envManager(),
                confTimePeriodManager()->activePeriodsMask())) {
        qCWarning(LC) << "Filter Simulator: Conf error:" << confBuf.errorMessage();
        return false;
    }

    const auto zones = taskManager()->taskInfoZoneDownloader()->loadZonesData();

    ConfBuffer zonesBuf;
    zonesBuf.writeZones(zones.dataZonesMask, zones.enabledMask, zones.dataSize, zones.zonesData);

    ConfBuffer rulesBuf;
    rulesBuf.writeRules(*confRuleManager());

    if (rulesBuf.hasError()) {
        qCWarning(LC) << "Filter Simulator: Rules error:" << rulesBuf.errorMessage();
        return false;
    }

    ConfBuffer groupsBuf;
    groupsBuf.writeGroups(*confGroupManager(), confGroupManager()->activeGroupsMask());

    // Filter the connection
    const char *drvConf = confBuf.data() + DriverCommon::confIoConfOff();

    const DriverCommon::ConnFilterConf cf = {
        .drvConfIo = confBuf.data(),
        .drvZones = zonesBuf.dataOrNull(),
        .drvRules = rulesBuf.dataOrNull(),
        .drvGroups = groupsBuf.dataOrNull(),
    };

    const FORT_APP_DATA appData = DriverCommon::confAppFind(drvConf, simConn.appPath);

    simConn.result = DriverCommon::confConnFilter(cf, &simConn.conn, appData);
    simConn.conn.app_data = appData;

    return true;
}

bool ConfManager::validateConf(const FirewallConf &conf)
{
    if (!conf.optEdited())
        return true;

    ConfBuffer confBuf;

    if (!confBuf.writeConf(conf, confAppManager(), envManager())) {
        qCCritical(LC) << "Conf save error:" << confBuf.errorMessage();
        return false;
    }

    return true;
}

bool ConfManager::validateDriver()
{
    ConfBuffer confBuf;

    confBuf.writeVersion();

    return driverManager()->validate(confBuf.buffer());
}

void ConfManager::updateServices()
{
    updateOwnProcessServices(serviceInfoManager());
}

void ConfManager::updateDriverServices(const QVector<ServiceInfo> &services, int processCount)
{
    ConfBuffer confBuf;

    confBuf.writeServices(services, processCount);

    driverManager()->writeServices(confBuf.buffer());
}

void ConfManager::updateOwnProcessServices(ServiceInfoManager *serviceInfoManager)
{
    const QVector<ServiceInfo> allServices =
            ServiceInfoManager::loadServiceInfoList(ServiceInfo::StateAll, /*displayName=*/false);

    serviceInfoManager->repairTrackedServices(allServices);

    int processCount = 0;
    const QVector<ServiceInfo> services =
            ServiceInfoManager::ownProcessServices(allServices, processCount);

    serviceInfoManager->monitorServices(services);

    if (processCount > 0) {
        updateDriverServices(services, processCount);
    }
}

bool ConfManager::loadFromDb(FirewallConf &conf, bool &isNew)
{
    // Load Address Groups
    {
        int count = 0;
        if (!loadAddressGroups(sqliteDb(), conf.addressGroups(), count))
            return false;

        if (count == 0) {
            isNew = true;
            return true;
        }
        isNew = false;
    }

    return true;
}

bool ConfManager::saveToDb(const FirewallConf &conf)
{
    beginWriteTransaction();

    bool ok = saveAddressGroups(sqliteDb(), conf); // Save Address Groups

    endTransaction(ok);

    return ok;
}

void ConfManager::saveTasksByIni(const IniOptions &ini)
{
    // Task Info List
    if (ini.taskInfoListSet()) {
        taskManager()->saveVariant(ini.taskInfoList());
    }
}

bool ConfManager::loadTask(TaskInfo *taskInfo)
{
    SqliteStmt stmt;
    if (!stmt.prepare(sqliteDb()->db(), sqlSelectTaskByName))
        return false;

    stmt.bindText(1, taskInfo->name());

    if (stmt.step() != SqliteStmt::StepRow)
        return false;

    taskInfo->setId(stmt.columnInt64(0));
    taskInfo->setEnabled(stmt.columnBool(1));
    taskInfo->setRunOnStartup(stmt.columnBool(2));
    taskInfo->setDelayStartup(stmt.columnBool(3));
    taskInfo->setMaxRetries(stmt.columnInt(4));
    taskInfo->setRetrySeconds(stmt.columnInt(5));
    taskInfo->setIntervalMinutes(stmt.columnInt(6));
    taskInfo->setLastRun(stmt.columnDateTime(7));
    taskInfo->setLastSuccess(stmt.columnDateTime(8));
    taskInfo->setData(stmt.columnBlob(9));

    return true;
}

bool ConfManager::saveTask(TaskInfo *taskInfo)
{
    const bool rowExists = (taskInfo->id() != 0);

    const QVariantList vars = {
        DbVar::nullable(taskInfo->id(), !rowExists),
        taskInfo->name(),
        taskInfo->enabled(),
        taskInfo->runOnStartup(),
        taskInfo->delayStartup(),
        taskInfo->maxRetries(),
        taskInfo->retrySeconds(),
        taskInfo->intervalMinutes(),
        taskInfo->lastRun(),
        taskInfo->lastSuccess(),
        taskInfo->data(),
    };

    const char *sql = rowExists ? sqlUpdateTask : sqlInsertTask;

    if (!DbQuery(sqliteDb()).sql(sql).vars(vars).executeOk())
        return false;

    if (!rowExists) {
        taskInfo->setId(sqliteDb()->lastInsertRowid());
    }
    return true;
}
