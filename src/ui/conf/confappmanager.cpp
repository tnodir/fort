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
#include <fortglobal.h>
#include <fortsettings.h>
#include <log/logentryapp.h>
#include <log/logmanager.h>
#include <manager/envmanager.h>
#include <util/conf/confbuffer.h>
#include <util/conf/confutil.h>
#include <util/dateutil.h>
#include <util/fileutil.h>
#include <util/variantutil.h>

#include "appgroup.h"
#include "confmanager.h"
#include "firewallconf.h"

using namespace Fort;

namespace {

const QLoggingCategory LC("confApp");

constexpr int APP_END_TIMER_INTERVAL_MIN = 100;
constexpr int APP_END_TIMER_INTERVAL_MAX = 24 * 60 * 60 * 1000; // 1 day

#define SELECT_APP_FIELDS                                                                          \
    "    t.app_id,"                                                                                \
    "    t.origin_path,"                                                                           \
    "    t.path,"                                                                                  \
    "    t.icon_path,"                                                                             \
    "    t.name,"                                                                                  \
    "    t.notes,"                                                                                 \
    "    t.is_wildcard,"                                                                           \
    "    t.apply_parent,"                                                                          \
    "    t.apply_child,"                                                                           \
    "    t.apply_spec_child,"                                                                      \
    "    t.kill_child,"                                                                            \
    "    t.lan_only,"                                                                              \
    "    t.parked,"                                                                                \
    "    t.log_stat,"                                                                              \
    "    t.log_allowed_conn,"                                                                      \
    "    t.log_blocked_conn,"                                                                      \
    "    t.blocked,"                                                                               \
    "    t.kill_process,"                                                                          \
    "    t.accept_zones,"                                                                          \
    "    t.reject_zones,"                                                                          \
    "    t.rule_id,"                                                                               \
    "    t.end_action,"                                                                            \
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

const char *const sqlSelectMinEndApp = "SELECT MIN(end_time) FROM app"
                                       "  WHERE end_time != 0;";

const char *const sqlSelectMaxAlertAppId = "SELECT MAX(app_id) FROM app_alert;";

const char *const sqlSelectEndedApps = "SELECT" SELECT_APP_FIELDS "  FROM app t"
                                       "    JOIN app_group g ON g.app_group_id = t.app_group_id"
                                       "    LEFT JOIN app_alert alert ON alert.app_id = t.app_id"
                                       "  WHERE end_time <= ?1;";

const char *const sqlSelectAppIdByPath = "SELECT app_id FROM app WHERE path = ?1;";

const char *const sqlUpsertApp = "INSERT INTO app(app_group_id, origin_path, path,"
                                 "    icon_path, name, notes, is_wildcard,"
                                 "    apply_parent, apply_child, apply_spec_child, kill_child,"
                                 "    lan_only, parked, log_stat, log_allowed_conn,"
                                 "    log_blocked_conn, blocked, kill_process,"
                                 "    accept_zones, reject_zones, rule_id,"
                                 "    end_action, end_time, creat_time)"
                                 "  VALUES(?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14,"
                                 "    ?15, ?16, ?17, ?18, ?19, ?20, ?21, ?22, ?23, ?24, ?25)"
                                 "  ON CONFLICT(path) DO UPDATE"
                                 "  SET app_group_id = ?2, origin_path = ?3, icon_path = ?5,"
                                 "    name = ?6, notes = ?7, is_wildcard = ?8,"
                                 "    apply_parent = ?9, apply_child = ?10, apply_spec_child = ?11,"
                                 "    kill_child = ?12, lan_only = ?13, parked = ?14,"
                                 "    log_stat = ?15, log_allowed_conn = ?16,"
                                 "    log_blocked_conn = ?17, blocked = ?18, kill_process = ?19,"
                                 "    accept_zones = ?20, reject_zones = ?21, rule_id = ?22,"
                                 "    end_action = ?23, end_time = ?24"
                                 "  RETURNING app_id;";

const char *const sqlUpdateApp = "UPDATE app"
                                 "  SET app_group_id = ?2, origin_path = ?3, path = ?4,"
                                 "    icon_path = ?5, name = ?6, notes = ?7, is_wildcard = ?8,"
                                 "    apply_parent = ?9, apply_child = ?10, apply_spec_child = ?11,"
                                 "    kill_child = ?12, lan_only = ?13, parked = ?14,"
                                 "    log_stat = ?15, log_allowed_conn = ?16,"
                                 "    log_blocked_conn = ?17, blocked = ?18, kill_process = ?19,"
                                 "    accept_zones = ?20, reject_zones = ?21, rule_id = ?22,"
                                 "    end_action = ?23, end_time = ?24"
                                 "  WHERE app_id = ?1"
                                 "  RETURNING app_id;";

const char *const sqlUpdateAppName = "UPDATE app SET name = ?2 WHERE app_id = ?1;";

const char *const sqlDeleteApp = "DELETE FROM app WHERE app_id = ?1 RETURNING is_wildcard, path;";

const char *const sqlInsertAppAlert = "INSERT INTO app_alert(app_id) VALUES(?1);";

const char *const sqlDeleteAppAlert = "DELETE FROM app_alert WHERE app_id = ?1;";

const char *const sqlSelectAppAlertIds = "SELECT app_id FROM app_alert"
                                         "  ORDER BY app_id DESC;";

const char *const sqlDeleteAppAlerts = "DELETE FROM app_alert;";

const char *const sqlUpdateAppBlocked = "UPDATE app SET blocked = ?2, kill_process = ?3"
                                        "  WHERE app_id = ?1;";

const char *const sqlUpdateAppTimer = "UPDATE app SET blocked = ?2, kill_process = ?3,"
                                      "    end_action = ?4, end_time = ?5"
                                      "  WHERE app_id = ?1;";

using AppsMap = QHash<qint64, QString>;
using AppIdsArray = QVector<qint64>;

}

ConfAppManager::ConfAppManager(QObject *parent) : ConfManagerBase(parent)
{
    connect(&m_appAlertedTimer, &QTimer::timeout, this, [&] { emit appAlerted(); });
    connect(&m_appsChangedTimer, &QTimer::timeout, this, &ConfAppManager::appsChanged);
    connect(&m_appUpdatedTimer, &QTimer::timeout, this, &ConfAppManager::appUpdated);

    m_appEndTimer.setSingleShot(true);
    connect(&m_appEndTimer, &QTimer::timeout, this, &ConfAppManager::updateAppEndTimes);
}

void ConfAppManager::setUp()
{
    setupConfManager();
    setupAppEndTimer();

    checkAppAlerted();
}

void ConfAppManager::setupConfManager()
{
    auto confManager = Fort::dependency<ConfManager>();

    connect(
            confManager, &ConfManager::confChanged, this,
            [&](bool /*onlyFlags*/, uint editedFlags) {
                if ((editedFlags & FirewallConf::AutoLearnOff) != 0
                        && ini().progRemoveLearntApps()) {
                    deleteAlertedApps();
                }
            },
            Qt::QueuedConnection);
}

void ConfAppManager::setupAppEndTimer()
{
    connect(logManager(), &LogManager::systemTimeChanged, this, &ConfAppManager::updateAppEndTimer);

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

void ConfAppManager::checkAppAlerted()
{
    if (getAlertAppId() != 0) {
        emitAppAlerted();
    }
}

bool ConfAppManager::addApp(App &app)
{
    app.appId = appIdByPath(app.appOriginPath, app.appPath);

    if (app.isValid())
        return false; // already exists

    app.appName = appInfoCache()->appName(app.appPath);

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
        DbVar::nullable(app.iconPath),
        app.appName,
        app.notes,
        app.isWildcard,
        app.applyParent,
        app.applyChild,
        app.applySpecChild,
        app.killChild,
        app.lanOnly,
        app.parked,
        app.logStat,
        app.logAllowedConn,
        app.logBlockedConn,
        app.blocked,
        app.killProcess,
        app.zones.accept_mask,
        app.zones.reject_mask,
        DbVar::nullable(app.ruleId),
        app.scheduleAction,
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

App ConfAppManager::appById(qint64 appId)
{
    App app;
    loadAppById(app, appId);
    return app;
}

App ConfAppManager::appByPath(const QString &appPath)
{
    QString normPath;
    const qint64 appId = appIdByPath(appPath, normPath);

    App app;
    if (!loadAppById(app, appId)) {
        app.appOriginPath = appPath;
        app.appPath = normPath;
    }
    return app;
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
    app.isWildcard = ConfUtil::hasWildcard(appOriginPath);
    app.blocked = blocked;
    app.appOriginPath = appOriginPath;

    bool ok = addApp(app);

    if (!ok && app.isValid()) {
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

    beginWriteTransaction();

    beginAddOrUpdateApp(app, *appGroup, onlyUpdate, ok);

    if (ok) {
        // Alert
        const char *sql = (app.alerted && !onlyUpdate) ? sqlInsertAppAlert : sqlDeleteAppAlert;

        DbQuery(sqliteDb()).sql(sql).vars({ app.appId }).executeOk();
    }

    endTransaction(ok);

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

    beginWriteTransaction();

    const QVariantList vars = { appId, appName };

    DbQuery(sqliteDb(), &ok).sql(sqlUpdateAppName).vars(vars).executeOk();

    endTransaction(ok);

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

bool ConfAppManager::addAlertedApp(qint64 appId)
{
    const bool ok = DbQuery(sqliteDb()).sql(sqlInsertAppAlert).vars({ appId }).executeOk();

    if (ok) {
        emitAppAlerted();
    }

    return ok;
}

bool ConfAppManager::deleteAlertedApps()
{
    QVector<qint64> appIdList;

    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sqlSelectAppAlertIds).prepare(stmt))
        return false;

    while (stmt.step() == SqliteStmt::StepRow) {
        const qint64 appId = stmt.columnInt64(0);
        appIdList.append(appId);
    }

    if (appIdList.isEmpty())
        return true;

    if (!deleteApps(appIdList))
        return false;

    emit appAlerted(/*alerted=*/false);

    return true;
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

    beginWriteTransaction();

    const QVariantList vars = { appId };

    const auto resList = DbQuery(sqliteDb(), &ok).sql(sqlDeleteApp).vars(vars).execute(2).toList();

    if (ok) {
        DbQuery(sqliteDb(), &ok).sql(sqlDeleteAppAlert).vars(vars).executeOk();
    }

    endTransaction(ok);

    if (ok && !resList.isEmpty()) {
        if (resList.at(0).toBool()) {
            isWildcard = true;
        } else {
            const QString appPath = resList.at(1).toString();

            updateDriverDeleteApp(appPath);
        }

        emitAppsChanged();

        emit appDeleted(appId);
    }

    return ok;
}

bool ConfAppManager::purgeApps()
{
    quint32 driveMask = -1;
    if (ini().progPurgeOnMounted()) {
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

bool ConfAppManager::updateAppsTimer(const QVector<qint64> &appIdList, int minutes)
{
    bool ok = true;
    bool isWildcard = false;

    const auto scheduleTime = (minutes > 0) ? DateUtil::now().addSecs(minutes * 60) : QDateTime();

    for (const qint64 appId : appIdList) {
        if (!updateAppTimer(appId, scheduleTime, isWildcard)) {
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
    if (!loadAppById(app, appId))
        return false;

    if (!checkAppBlockedChanged(app, blocked, killProcess))
        return true;

    if (!saveAppBlocked(app))
        return false;

    if (isWildcard || app.isWildcard) {
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

bool ConfAppManager::updateAppTimer(qint64 appId, QDateTime scheduleTime, bool &isWildcard)
{
    App app;
    if (!loadAppById(app, appId))
        return false;

    const bool timerDisabled = scheduleTime.isNull();

    app.blocked = timerDisabled;
    app.killProcess = false;
    app.scheduleAction = timerDisabled ? App::ScheduleAllow : App::ScheduleBlock;
    app.scheduleTime = scheduleTime;

    if (!saveAppTimer(app))
        return false;

    if (isWildcard || app.isWildcard) {
        isWildcard = true;
    } else {
        updateDriverUpdateApp(app);
    }

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

    beginWriteTransaction();

    const QVariantList vars = { app.appId, app.blocked, app.killProcess };

    DbQuery(sqliteDb(), &ok).sql(sqlUpdateAppBlocked).vars(vars).executeOk();

    if (ok) {
        DbQuery(sqliteDb(), &ok).sql(sqlDeleteAppAlert).vars({ app.appId }).executeOk();
    }

    endTransaction(ok);

    if (ok) {
        emitAppUpdated();
    }

    return ok;
}

bool ConfAppManager::saveAppTimer(const App &app)
{
    bool ok = true;

    const QVariantList vars = {
        app.appId,
        app.blocked,
        app.killProcess,
        app.scheduleAction,
        DbVar::nullable(app.scheduleTime),
    };

    DbQuery(sqliteDb(), &ok).sql(sqlUpdateAppTimer).vars(vars).executeOk();

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
                                " lan_only, parked, log_stat, log_allowed_conn, log_blocked_conn,"
                                " blocked, kill_process, end_action, end_time, creat_time";

    const QString sql = "INSERT INTO app(app_group_id, " + columnNames + ") SELECT 1, "
            + columnNames + " FROM " + schemaApp
            + " ba WHERE NOT EXISTS (SELECT 1 FROM app WHERE path = ba.path);";

    beginWriteTransaction();

    const qint64 oldMaxAppId =
            DbQuery(sqliteDb()).sql("SELECT MAX(app_id) FROM app;").execute().toLongLong();

    // Import apps
    bool ok = DbQuery(sqliteDb()).sql(sql).executeOk();

    // Add alerts
    if (ok) {
        DbQuery(sqliteDb(), &ok)
                .sql("INSERT INTO app_alert(app_id) SELECT app_id FROM app WHERE app_id > ?1;")
                .vars({ oldMaxAppId })
                .executeOk();
    }

    endTransaction(ok);

    if (!ok) {
        qCWarning(LC) << "Import apps backup error:" << sqliteDb()->errorMessage();
    }

    sqliteDb()->detach(schemaName);

    if (ok) {
        emitAppsChanged();
    }

    return ok;
}

bool ConfAppManager::updateDriverConf(bool onlyFlags)
{
    ConfBuffer confBuf;

    const auto &conf = *Fort::conf();

    const bool ok = onlyFlags ? (confBuf.writeFlags(conf), true)
                              : confBuf.writeConf(conf, this, envManager());

    if (!ok) {
        qCWarning(LC) << "Driver config error:" << confBuf.errorMessage();
        return false;
    }

    auto driverManager = Fort::driverManager();

    if (!driverManager->writeConf(confBuf.buffer(), onlyFlags)) {
        qCWarning(LC) << "Update driver error:" << driverManager->errorMessage();
        return false;
    }

    return true;
}

bool ConfAppManager::loadAppById(App &app, qint64 appId)
{
    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sqlSelectAppById).vars({ appId }).prepareRow(stmt))
        return false;

    fillApp(app, stmt);

    return true;
}

void ConfAppManager::fillApp(App &app, const SqliteStmt &stmt)
{
    app.appId = stmt.columnInt64(0);
    app.appOriginPath = stmt.columnText(1);
    app.appPath = stmt.columnText(2);
    app.iconPath = stmt.columnText(3);
    app.appName = stmt.columnText(4);
    app.notes = stmt.columnText(5);
    app.isWildcard = stmt.columnBool(6);
    app.applyParent = stmt.columnBool(7);
    app.applyChild = stmt.columnBool(8);
    app.applySpecChild = stmt.columnBool(9);
    app.killChild = stmt.columnBool(10);
    app.lanOnly = stmt.columnBool(11);
    app.parked = stmt.columnBool(12);
    app.logStat = stmt.columnBool(13);
    app.logAllowedConn = stmt.columnBool(14);
    app.logBlockedConn = stmt.columnBool(15);
    app.blocked = stmt.columnBool(16);
    app.killProcess = stmt.columnBool(17);
    app.zones.accept_mask = stmt.columnUInt(18);
    app.zones.reject_mask = stmt.columnUInt(19);
    app.ruleId = stmt.columnUInt(20);
    app.scheduleAction = stmt.columnInt(21);
    app.scheduleTime = stmt.columnDateTime(22);
    app.groupIndex = stmt.columnInt(23);
    app.alerted = stmt.columnBool(24);
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

    auto driverManager = Fort::driverManager();

    if (!driverManager->writeApp(confBuf.buffer(), remove)) {
        qCWarning(LC) << "Update driver error:" << driverManager->errorMessage();
        return false;
    }

    return true;
}

bool ConfAppManager::updateDriverUpdateAppConf(const App &app)
{
    return app.isWildcard ? updateDriverConf() : updateDriverUpdateApp(app);
}
