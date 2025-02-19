#include "confappmanager.h"

#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/dbvar.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <appinfo/appinfocache.h>
#include <appinfo/appinfoutil.h>
#include <conf/app.h>
#include <driver/drivermanager.h>
#include <log/logentryapp.h>
#include <log/logmanager.h>
#include <manager/drivelistmanager.h>
#include <manager/envmanager.h>
#include <stat/statmanager.h>
#include <util/conf/confbuffer.h>
#include <util/dateutil.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>

#include "appgroup.h"
#include "confmanager.h"
#include "firewallconf.h"

namespace {

const QLoggingCategory LC("confApp");

constexpr int APP_END_TIMER_INTERVAL_MIN = 100;
constexpr int APP_END_TIMER_INTERVAL_MAX = 24 * 60 * 60 * 1000; // 1 day

#define SELECT_APP_FIELDS                                                                          \
    "    t.app_id,"                                                                                \
    "    t.origin_path,"                                                                           \
    "    t.path,"                                                                                  \
    "    t.name,"                                                                                  \
    "    t.notes,"                                                                                 \
    "    t.is_wildcard,"                                                                           \
    "    t.apply_parent,"                                                                          \
    "    t.apply_child,"                                                                           \
    "    t.apply_spec_child,"                                                                      \
    "    t.kill_child,"                                                                            \
    "    t.lan_only,"                                                                              \
    "    t.parked,"                                                                                \
    "    t.log_allowed_conn,"                                                                      \
    "    t.log_blocked_conn,"                                                                      \
    "    t.blocked,"                                                                               \
    "    t.kill_process,"                                                                          \
    "    t.accept_zones,"                                                                          \
    "    t.reject_zones,"                                                                          \
    "    t.rule_id,"                                                                               \
    "    t.end_action,"                                                                            \
    "    t.end_event,"                                                                             \
    "    t.end_time,"                                                                              \
    "    g.order_index as group_index,"                                                            \
    "    (alert.app_id IS NOT NULL) as alerted"

const char *const sqlSelectAppById = "SELECT" SELECT_APP_FIELDS "  FROM app t"
                                     "    JOIN app_group g ON g.app_group_id = t.app_group_id"
                                     "    LEFT JOIN app_alert alert ON alert.app_id = t.app_id"
                                     "    WHERE t.app_id = ?1;";

const char *const sqlSelectApps = "SELECT" SELECT_APP_FIELDS "  FROM app t"
                                  "    JOIN app_group g ON g.app_group_id = t.app_group_id"
                                  "    LEFT JOIN app_alert alert ON alert.app_id = t.app_id"
                                  "  ORDER BY t.path;";

const char *const sqlSelectAppsToPurge = "SELECT app_id, path FROM app"
                                         "  WHERE is_wildcard = 0 AND parked = 0;";

const char *const sqlDeleteAppsEndEvent = "DELETE FROM app WHERE end_event <> 0;";

const char *const sqlSelectAppIdByPathEndEvent = "SELECT app_id FROM app"
                                                 "  WHERE path = ?1 AND end_event = ?2;";

const char *const sqlSelectMinEndApp = "SELECT MIN(end_time) FROM app"
                                       "  WHERE end_time != 0;";

const char *const sqlSelectMaxAlertAppId = "SELECT MAX(app_id) FROM app_alert;";

const char *const sqlSelectEndedApps = "SELECT" SELECT_APP_FIELDS "  FROM app t"
                                       "    JOIN app_group g ON g.app_group_id = t.app_group_id"
                                       "    LEFT JOIN app_alert alert ON alert.app_id = t.app_id"
                                       "  WHERE end_time <= ?1;";

const char *const sqlSelectAppIdByPath = "SELECT app_id FROM app WHERE path = ?1;";

const char *const sqlUpsertApp = "INSERT INTO app(app_group_id, origin_path, path,"
                                 "    name, notes, is_wildcard,"
                                 "    apply_parent, apply_child, apply_spec_child, kill_child,"
                                 "    lan_only, parked, log_allowed_conn, log_blocked_conn,"
                                 "    blocked, kill_process, accept_zones, reject_zones,"
                                 "    rule_id, end_action, end_event, end_time, creat_time)"
                                 "  VALUES(?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14,"
                                 "    ?15, ?16, ?17, ?18, ?19, ?20, ?21, ?22, ?23, ?24)"
                                 "  ON CONFLICT(path) DO UPDATE"
                                 "  SET app_group_id = ?2, origin_path = ?3,"
                                 "    name = ?5, notes = ?6, is_wildcard = ?7,"
                                 "    apply_parent = ?8, apply_child = ?9, apply_spec_child = ?10,"
                                 "    kill_child = ?11, lan_only = ?12, parked = ?13,"
                                 "    log_allowed_conn = ?14, log_blocked_conn = ?15,"
                                 "    blocked = ?16, kill_process = ?17,"
                                 "    accept_zones = ?18, reject_zones = ?19, rule_id = ?20,"
                                 "    end_action = ?21, end_event = ?22, end_time = ?23"
                                 "  RETURNING app_id;";

const char *const sqlUpdateApp = "UPDATE app"
                                 "  SET app_group_id = ?2, origin_path = ?3, path = ?4,"
                                 "    name = ?5, notes = ?6, is_wildcard = ?7,"
                                 "    apply_parent = ?8, apply_child = ?9, apply_spec_child = ?10,"
                                 "    kill_child = ?11, lan_only = ?12, parked = ?13,"
                                 "    log_allowed_conn = ?14, log_blocked_conn = ?15,"
                                 "    blocked = ?16, kill_process = ?17,"
                                 "    accept_zones = ?18, reject_zones = ?19, rule_id = ?20,"
                                 "    end_action = ?21, end_event = ?22, end_time = ?23"
                                 "  WHERE app_id = ?1"
                                 "  RETURNING app_id;";

const char *const sqlUpdateAppName = "UPDATE app SET name = ?2 WHERE app_id = ?1;";

const char *const sqlDeleteApp = "DELETE FROM app WHERE app_id = ?1 RETURNING is_wildcard, path;";

const char *const sqlInsertAppAlert = "INSERT INTO app_alert(app_id) VALUES(?1);";

const char *const sqlDeleteAppAlert = "DELETE FROM app_alert WHERE app_id = ?1;";

const char *const sqlDeleteAppAlerts = "DELETE FROM app_alert;";

const char *const sqlUpdateAppBlocked = "UPDATE app SET blocked = ?2, kill_process = ?3"
                                        "  WHERE app_id = ?1;";

using AppsMap = QHash<qint64, QString>;
using AppIdsArray = QVector<qint64>;

}

ConfAppManager::ConfAppManager(QObject *parent) : QObject(parent)
{
    connect(&m_appAlertedTimer, &QTimer::timeout, this, &ConfAppManager::appAlerted);
    connect(&m_appsChangedTimer, &QTimer::timeout, this, &ConfAppManager::appsChanged);
    connect(&m_appUpdatedTimer, &QTimer::timeout, this, &ConfAppManager::appUpdated);

    m_appEndTimer.setSingleShot(true);
    connect(&m_appEndTimer, &QTimer::timeout, this, &ConfAppManager::updateAppEndTimes);
}

ConfManager *ConfAppManager::confManager() const
{
    return m_confManager;
}

SqliteDb *ConfAppManager::sqliteDb() const
{
    return confManager()->sqliteDb();
}

FirewallConf *ConfAppManager::conf() const
{
    return confManager()->conf();
}

void ConfAppManager::setUp()
{
    m_confManager = IoCDependency<ConfManager>();

    setupDriveListManager();

    setupAppEndEvent();
    setupAppEndTimer();
}

void ConfAppManager::setupDriveListManager()
{
    connect(IoC<DriveListManager>(), &DriveListManager::driveMaskChanged, this,
            [&](quint32 addedMask, quint32 /*removedMask*/) {
                if ((m_driveMask & addedMask) != 0) {
                    updateDriverConf();
                }
            });
}

void ConfAppManager::setupAppEndEvent()
{
    DbQuery(sqliteDb()).sql(sqlDeleteAppsEndEvent).executeOk();

    setupAppEndEvent_onConnectionsClosed();
}

void ConfAppManager::setupAppEndEvent_onProcessExit()
{
    //
}

void ConfAppManager::setupAppEndEvent_onConnectionsClosed()
{
    const auto onAppProcessIdRemoved = [&](qint32 /*pid*/, const QString &appPath) {
        // const QString normPath = FileUtil::normalizePath(appPath);

        const qint64 appId = DbQuery(sqliteDb())
                                     .sql(sqlSelectAppIdByPathEndEvent)
                                     .vars({ appPath, App::ScheduleOnConnectionsClosed })
                                     .execute()
                                     .toLongLong();
        if (appId > 0) {
            deleteAppPath(appPath);
        }
    };

    auto statManager = IoCDependency<StatManager>();

    connect(statManager, &StatManager::appProcessIdRemoved, this, onAppProcessIdRemoved);
}

void ConfAppManager::setupAppEndTimer()
{
    auto logManager = IoC<LogManager>();

    connect(logManager, &LogManager::systemTimeChanged, this, &ConfAppManager::updateAppEndTimer);

    updateAppEndTimer();
}

void ConfAppManager::updateAppEndTimer()
{
    m_appEndTimer.stop();

    const qint64 endTimeMsecs = DbQuery(sqliteDb()).sql(sqlSelectMinEndApp).execute().toLongLong();
    if (endTimeMsecs == 0)
        return;

    const qint64 currentMsecs = QDateTime::currentMSecsSinceEpoch();
    const qint64 deltaMsecs = endTimeMsecs - currentMsecs;
    const int interval = qBound(
            qint64(APP_END_TIMER_INTERVAL_MIN), deltaMsecs, qint64(APP_END_TIMER_INTERVAL_MAX));

    m_appEndTimer.start(interval);
}

bool ConfAppManager::addApp(App &app)
{
    app.appId = appIdByPath(app.appOriginPath, app.appPath);

    if (app.appId > 0)
        return false; // already exists

    app.appName = IoC<AppInfoCache>()->appName(app.appPath);

    const bool ok = addOrUpdateApp(app);

    if (ok && app.alerted) {
        emitAppAlerted();
    }

    return ok;
}

void ConfAppManager::beginAddOrUpdateApp(
        App &app, const AppGroup &appGroup, bool onlyUpdate, bool &ok)
{
    const QVariantList vars = {
        app.appId,
        appGroup.id(),
        app.appOriginPath,
        DbVar::nullable(app.appPath),
        app.appName,
        app.notes,
        app.isWildcard,
        app.applyParent,
        app.applyChild,
        app.applySpecChild,
        app.killChild,
        app.lanOnly,
        app.parked,
        app.logAllowedConn,
        app.logBlockedConn,
        app.blocked,
        app.killProcess,
        app.acceptZones,
        app.rejectZones,
        DbVar::nullable(app.ruleId),
        app.scheduleAction,
        app.scheduleEvent,
        DbVar::nullable(app.scheduleTime),
        DbVar::nullable(DateUtil::now(), onlyUpdate),
    };

    const char *sql = onlyUpdate ? sqlUpdateApp : sqlUpsertApp;
    const auto appIdVar = DbQuery(sqliteDb(), &ok).sql(sql).vars(vars).execute();

    if (!onlyUpdate) {
        app.appId = appIdVar.toLongLong();
    }
}

void ConfAppManager::endAddOrUpdateApp(const App &app, bool onlyUpdate)
{
    if (!app.scheduleTime.isNull()) {
        updateAppEndTimer();
    }

    if (onlyUpdate) {
        emitAppUpdated();
    } else {
        emitAppsChanged();
    }

    updateDriverUpdateAppConf(app);
}

void ConfAppManager::emitAppAlerted()
{
    m_appAlertedTimer.startTrigger();
}

void ConfAppManager::emitAppsChanged()
{
    m_appsChangedTimer.startTrigger();
}

void ConfAppManager::emitAppUpdated()
{
    m_appUpdatedTimer.startTrigger();
}

void ConfAppManager::logApp(const LogEntryApp &logEntry)
{
    App app;
    app.blocked = logEntry.blocked();
    app.alerted = logEntry.alerted();
    app.appOriginPath = FileUtil::realPath(logEntry.path());
    app.scheduleAction = App::ScheduleRemove; // default action for alert

    addApp(app);
}

qint64 ConfAppManager::appIdByPath(const QString &appOriginPath, QString &normPath)
{
    normPath = FileUtil::normalizePath(appOriginPath);

    return DbQuery(sqliteDb()).sql(sqlSelectAppIdByPath).vars({ normPath }).execute().toLongLong();
}

bool ConfAppManager::addOrUpdateAppPath(
        const QString &appOriginPath, bool blocked, bool killProcess)
{
    App app;
    app.blocked = blocked;
    app.appOriginPath = appOriginPath;

    bool ok = addApp(app);

    if (!ok && app.appId > 0) {
        ok = updateAppsBlocked({ app.appId }, blocked, killProcess);
    }

    return ok;
}

bool ConfAppManager::deleteAppPath(const QString &appOriginPath)
{
    QString normPath;
    const qint64 appId = appIdByPath(appOriginPath, normPath);

    if (appId <= 0)
        return false; // does not exist

    return deleteApps({ appId });
}

bool ConfAppManager::addOrUpdateApp(App &app, bool onlyUpdate)
{
    const AppGroup *appGroup = conf()->appGroupAt(app.groupIndex);
    if (appGroup->isNull())
        return false;

    bool ok = false;

    beginTransaction();

    beginAddOrUpdateApp(app, *appGroup, onlyUpdate, ok);

    if (ok) {
        // Alert
        const char *sql = (app.alerted && !onlyUpdate) ? sqlInsertAppAlert : sqlDeleteAppAlert;

        DbQuery(sqliteDb()).sql(sql).vars({ app.appId }).executeOk();
    }

    commitTransaction(ok);

    if (ok) {
        endAddOrUpdateApp(app, onlyUpdate);
    }

    return ok;
}

bool ConfAppManager::updateApp(App &app)
{
    return addOrUpdateApp(app, /*onlyUpdate=*/true);
}

bool ConfAppManager::updateAppName(qint64 appId, const QString &appName)
{
    bool ok = false;

    beginTransaction();

    const QVariantList vars = { appId, appName };

    DbQuery(sqliteDb(), &ok).sql(sqlUpdateAppName).vars(vars).executeOk();

    commitTransaction(ok);

    if (ok) {
        emitAppUpdated();
    }

    return ok;
}

bool ConfAppManager::deleteApps(const QVector<qint64> &appIdList)
{
    bool ok = true;
    bool isWildcard = false;

    for (const qint64 appId : appIdList) {
        if (!deleteApp(appId, isWildcard)) {
            ok = false;
            break;
        }
    }

    if (isWildcard) {
        updateDriverConf();
    }

    return ok;
}

bool ConfAppManager::clearAlerts()
{
    const bool ok = DbQuery(sqliteDb()).sql(sqlDeleteAppAlerts).executeOk();

    if (ok) {
        emitAppUpdated();
    }

    return ok;
}

bool ConfAppManager::deleteApp(qint64 appId, bool &isWildcard)
{
    bool ok = false;

    beginTransaction();

    const QVariantList vars = { appId };

    const auto resList = DbQuery(sqliteDb(), &ok).sql(sqlDeleteApp).vars(vars).execute(2).toList();

    if (ok) {
        DbQuery(sqliteDb(), &ok).sql(sqlDeleteAppAlert).vars(vars).executeOk();
    }

    commitTransaction(ok);

    if (ok) {
        if (resList.at(0).toBool()) {
            isWildcard = true;
        } else {
            const QString appPath = resList.at(1).toString();

            updateDriverDeleteApp(appPath);
        }

        emitAppsChanged();
    }

    return ok;
}

bool ConfAppManager::purgeApps()
{
    quint32 driveMask = -1;
    if (conf()->ini().progPurgeOnMounted()) {
        driveMask = FileUtil::mountedDriveMask(FileUtil::driveMask());
    }

    const auto appIdList = collectObsoleteApps(driveMask);
    if (appIdList.isEmpty())
        return true;

    // Delete obsolete apps
    return deleteApps(appIdList);
}

bool ConfAppManager::updateAppsBlocked(
        const QVector<qint64> &appIdList, bool blocked, bool killProcess)
{
    bool ok = true;
    bool isWildcard = (appIdList.size() > 7);

    for (const qint64 appId : appIdList) {
        if (!updateAppBlocked(appId, blocked, killProcess, isWildcard)) {
            ok = false;
            break;
        }
    }

    if (isWildcard) {
        updateDriverConf();
    }

    return ok;
}

bool ConfAppManager::updateAppBlocked(
        qint64 appId, bool blocked, bool killProcess, bool &isWildcard)
{
    App app;
    app.appId = appId;
    if (!loadAppById(app))
        return false;

    if (!checkAppBlockedChanged(app, blocked, killProcess))
        return true;

    if (!saveAppBlocked(app))
        return false;

    if (app.isWildcard) {
        isWildcard = true;
    } else {
        updateDriverUpdateApp(app);
    }

    return true;
}

bool ConfAppManager::checkAppBlockedChanged(App &app, bool blocked, bool killProcess)
{
    const bool wasAlerted = app.alerted;
    app.alerted = false;

    if (!wasAlerted) {
        if (app.blocked == blocked && app.killProcess == killProcess)
            return false;
    }

    app.blocked = blocked;
    app.killProcess = killProcess;

    return true;
}

QVector<qint64> ConfAppManager::collectObsoleteApps(quint32 driveMask)
{
    QVector<qint64> appIdList;

    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sqlSelectAppsToPurge).prepare(stmt))
        return {};

    while (stmt.step() == SqliteStmt::StepRow) {
        const QString appPath = stmt.columnText(1);

        const quint32 mask = FileUtil::driveMaskByPath(appPath);
        if ((mask & driveMask) == 0)
            continue; // skip non-path or not-mounted

        if (!AppInfoUtil::fileExists(appPath)) {
            const qint64 appId = stmt.columnInt64(0);
            appIdList.append(appId);

            qCInfo(LC) << "Obsolete app:" << appId << appPath;
        }
    }

    return appIdList;
}

bool ConfAppManager::walkApps(const std::function<walkAppsCallback> &func) const
{
    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sqlSelectApps).prepare(stmt))
        return false;

    while (stmt.step() == SqliteStmt::StepRow) {
        App app;
        fillApp(app, stmt);

        if (!func(app))
            return false;
    }

    return true;
}

bool ConfAppManager::saveAppBlocked(const App &app)
{
    bool ok = true;

    beginTransaction();

    const QVariantList vars = { app.appId, app.blocked, app.killProcess };

    DbQuery(sqliteDb(), &ok).sql(sqlUpdateAppBlocked).vars(vars).executeOk();

    if (ok) {
        DbQuery(sqliteDb(), &ok).sql(sqlDeleteAppAlert).vars({ app.appId }).executeOk();
    }

    commitTransaction(ok);

    if (ok) {
        emitAppUpdated();
    }

    return ok;
}

void ConfAppManager::updateAppEndTimes()
{
    QVector<qint64> appIdListToRemove;

    SqliteStmt stmt;
    if (!stmt.prepare(sqliteDb()->db(), sqlSelectEndedApps))
        return;

    stmt.bindDateTime(1, DateUtil::now());

    while (stmt.step() == SqliteStmt::StepRow) {
        App app;
        fillApp(app, stmt);

        if (app.scheduleAction == App::ScheduleRemove) {
            appIdListToRemove.append(app.appId);
        } else {
            app.blocked = (app.scheduleAction != App::ScheduleAllow);
            app.killProcess = (app.scheduleAction == App::ScheduleKillProcess);
            app.scheduleTime = {};

            updateApp(app);
        }
    }

    if (!appIdListToRemove.isEmpty()) {
        deleteApps(appIdListToRemove);
    }

    updateAppEndTimer();
}

qint64 ConfAppManager::getAlertAppId()
{
    return DbQuery(sqliteDb()).sql(sqlSelectMaxAlertAppId).execute().toLongLong();
}

bool ConfAppManager::importAppsBackup(const QString &path)
{
    const auto backupFilePath =
            FileUtil::pathSlash(path) + FileUtil::fileName(sqliteDb()->filePath());

    const QLatin1String schemaName("backup");

    if (!sqliteDb()->attach(schemaName, backupFilePath)) {
        qCWarning(LC) << "Open backup error:" << sqliteDb()->errorMessage() << backupFilePath;
        return false;
    }

    const QString schemaApp = SqliteDb::entityName(schemaName, "app");

    const QString columnNames = "origin_path, path, name, notes, is_wildcard,"
                                " apply_parent, apply_child, apply_spec_child, kill_child,"
                                " lan_only, parked, log_allowed_conn, log_blocked_conn,"
                                " blocked, kill_process, end_action, end_time, creat_time";

    const QString sql = "INSERT INTO app(app_group_id, " + columnNames + ") SELECT 1, "
            + columnNames + " FROM " + schemaApp
            + " ba WHERE NOT EXISTS (SELECT 1 FROM app WHERE path = ba.path);";

    sqliteDb()->beginWriteTransaction();

    const qint64 oldMaxAppId =
            DbQuery(sqliteDb()).sql("SELECT MAX(app_id) FROM app;").execute().toLongLong();

    // Import apps
    const bool ok = DbQuery(sqliteDb()).sql(sql).executeOk();

    // Add alerts
    if (ok) {
        DbQuery(sqliteDb())
                .sql("INSERT INTO app_alert(app_id) SELECT app_id FROM app WHERE app_id > ?1;")
                .vars({ oldMaxAppId })
                .executeOk();
    }

    if (!ok) {
        qCWarning(LC) << "Import apps backup error:" << sqliteDb()->errorMessage();
    }

    sqliteDb()->commitTransaction();

    sqliteDb()->detach(schemaName);

    if (ok) {
        emitAppsChanged();
    }

    return ok;
}

bool ConfAppManager::updateDriverConf(bool onlyFlags)
{
    ConfBuffer confBuf;

    const bool ok = onlyFlags ? (confBuf.writeFlags(*conf()), true)
                              : confBuf.writeConf(*conf(), this, *IoC<EnvManager>());

    if (!ok) {
        qCWarning(LC) << "Driver config error:" << confBuf.errorMessage();
        return false;
    }

    auto driverManager = IoC<DriverManager>();
    if (!driverManager->writeConf(confBuf.buffer(), onlyFlags)) {
        qCWarning(LC) << "Update driver error:" << driverManager->errorMessage();
        return false;
    }

    m_driveMask = confBuf.driveMask();

    return true;
}

bool ConfAppManager::loadAppById(App &app)
{
    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sqlSelectAppById).prepare(stmt))
        return false;

    stmt.bindInt64(1, app.appId);
    if (stmt.step() != SqliteStmt::StepRow)
        return false;

    fillApp(app, stmt);

    return true;
}

void ConfAppManager::fillApp(App &app, const SqliteStmt &stmt)
{
    app.appId = stmt.columnInt64(0);
    app.appOriginPath = stmt.columnText(1);
    app.appPath = stmt.columnText(2);
    app.appName = stmt.columnText(3);
    app.notes = stmt.columnText(4);
    app.isWildcard = stmt.columnBool(5);
    app.applyParent = stmt.columnBool(6);
    app.applyChild = stmt.columnBool(7);
    app.applySpecChild = stmt.columnBool(8);
    app.killChild = stmt.columnBool(9);
    app.lanOnly = stmt.columnBool(10);
    app.parked = stmt.columnBool(11);
    app.logAllowedConn = stmt.columnBool(12);
    app.logBlockedConn = stmt.columnBool(13);
    app.blocked = stmt.columnBool(14);
    app.killProcess = stmt.columnBool(15);
    app.acceptZones = stmt.columnUInt(16);
    app.rejectZones = stmt.columnUInt(17);
    app.ruleId = stmt.columnUInt(18);
    app.scheduleAction = stmt.columnInt(19);
    app.scheduleEvent = stmt.columnInt(20);
    app.scheduleTime = stmt.columnDateTime(21);
    app.groupIndex = stmt.columnInt(22);
    app.alerted = stmt.columnBool(23);
}

bool ConfAppManager::updateDriverDeleteApp(const QString &appPath)
{
    App app;
    app.appPath = appPath;

    return updateDriverUpdateApp(app, /*remove=*/true);
}

bool ConfAppManager::updateDriverUpdateApp(const App &app, bool remove)
{
    ConfBuffer confBuf;

    if (!confBuf.writeAppEntry(app)) {
        qCWarning(LC) << "Driver config error:" << confBuf.errorMessage();
        return false;
    }

    auto driverManager = IoC<DriverManager>();
    if (!driverManager->writeApp(confBuf.buffer(), remove)) {
        qCWarning(LC) << "Update driver error:" << driverManager->errorMessage();
        return false;
    }

    m_driveMask |= remove ? 0 : confBuf.driveMask();

    return true;
}

bool ConfAppManager::updateDriverUpdateAppConf(const App &app)
{
    return app.isWildcard ? updateDriverConf() : updateDriverUpdateApp(app);
}

bool ConfAppManager::beginTransaction()
{
    return sqliteDb()->beginWriteTransaction();
}

void ConfAppManager::commitTransaction(bool &ok)
{
    ok = sqliteDb()->endTransaction(ok);
}
