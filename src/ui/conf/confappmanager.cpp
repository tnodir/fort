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
#include <log/logentryblocked.h>
#include <log/logmanager.h>
#include <manager/drivelistmanager.h>
#include <manager/envmanager.h>
#include <util/conf/confutil.h>
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
    "    t.use_group_perm,"                                                                        \
    "    t.apply_child,"                                                                           \
    "    t.kill_child,"                                                                            \
    "    t.lan_only,"                                                                              \
    "    t.parked,"                                                                                \
    "    t.log_blocked,"                                                                           \
    "    t.log_conn,"                                                                              \
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

const char *const sqlUpsertApp = "INSERT INTO app(app_group_id, origin_path, path, name, notes,"
                                 "    is_wildcard, use_group_perm, apply_child, kill_child,"
                                 "    lan_only, parked, log_blocked, log_conn,"
                                 "    blocked, kill_process, accept_zones, reject_zones,"
                                 "    rule_id, end_action, end_time, creat_time)"
                                 "  VALUES(?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14,"
                                 "    ?15, ?16, ?17, ?18, ?19, ?20, ?21, ?22)"
                                 "  ON CONFLICT(path) DO UPDATE"
                                 "  SET app_group_id = ?2, origin_path = ?3, name = ?5,"
                                 "    notes = ?6, is_wildcard = ?7, use_group_perm = ?8,"
                                 "    apply_child = ?9, kill_child = ?10, lan_only = ?11,"
                                 "    parked = ?12, log_blocked = ?13, log_conn = ?14,"
                                 "    blocked = ?15, kill_process = ?16,"
                                 "    accept_zones = ?17, reject_zones = ?18, rule_id = ?19,"
                                 "    end_action = ?20, end_time = ?21"
                                 "  RETURNING app_id;";

const char *const sqlUpdateApp = "UPDATE app"
                                 "  SET app_group_id = ?2, origin_path = ?3, path = ?4, name = ?5,"
                                 "    notes = ?6, is_wildcard = ?7, use_group_perm = ?8,"
                                 "    apply_child = ?9, kill_child = ?10, lan_only = ?11,"
                                 "    parked = ?12, log_blocked = ?13, log_conn = ?14,"
                                 "    blocked = ?15, kill_process = ?16,"
                                 "    accept_zones = ?17, reject_zones = ?18, rule_id = ?19,"
                                 "    end_action = ?20, end_time = ?21"
                                 "  WHERE app_id = ?1"
                                 "  RETURNING app_id;";

const char *const sqlUpdateAppName = "UPDATE app SET name = ?2 WHERE app_id = ?1;";

const char *const sqlDeleteApp = "DELETE FROM app WHERE app_id = ?1 RETURNING path, is_wildcard;";

const char *const sqlInsertAppAlert = "INSERT INTO app_alert(app_id) VALUES(?1);";

const char *const sqlDeleteAppAlert = "DELETE FROM app_alert WHERE app_id = ?1;";

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

void ConfAppManager::setupAppEndTimer()
{
    auto logManager = IoC<LogManager>();

    connect(logManager, &LogManager::systemTimeChanged, this, &ConfAppManager::updateAppEndTimer);

    updateAppEndTimer();
}

void ConfAppManager::updateAppEndTimer()
{
    const qint64 endTimeMsecs = DbQuery(sqliteDb()).sql(sqlSelectMinEndApp).execute().toLongLong();

    if (endTimeMsecs != 0) {
        const qint64 currentMsecs = QDateTime::currentMSecsSinceEpoch();
        const qint64 deltaMsecs = endTimeMsecs - currentMsecs;
        const int interval = qMax(
                (deltaMsecs > 0 ? int(qMin(deltaMsecs, qint64(APP_END_TIMER_INTERVAL_MAX))) : 0),
                APP_END_TIMER_INTERVAL_MIN);

        m_appEndTimer.start(interval);
    } else {
        m_appEndTimer.stop();
    }
}

bool ConfAppManager::addAppPathBlocked(App &app)
{
    app.appId = appIdByPath(app.appOriginPath, app.appPath);

    if (app.appId > 0)
        return false; // already exists

    app.isWildcard = ConfUtil::matchWildcard(app.appPath).hasMatch();
    app.appName = app.isWildcard ? app.appOriginPath : IoC<AppInfoCache>()->appName(app.appPath);

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
        app.useGroupPerm,
        app.applyChild,
        app.killChild,
        app.lanOnly,
        app.parked,
        app.logBlocked,
        app.logConn,
        app.blocked,
        app.killProcess,
        app.acceptZones,
        app.rejectZones,
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

void ConfAppManager::logBlockedApp(const LogEntryBlocked &logEntry)
{
    App app;
    app.blocked = logEntry.blocked();
    app.alerted = logEntry.alerted();
    app.appOriginPath = logEntry.path();
    app.scheduleAction = App::ScheduleRemove; // default action for alert

    addAppPathBlocked(app);
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

    bool ok = addAppPathBlocked(app);

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
        const QString appPath = resList.at(0).toString();

        if (resList.at(1).toBool()) {
            isWildcard = true;
        } else {
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

            qCDebug(LC) << "Obsolete app:" << appId << appPath;
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

bool ConfAppManager::updateDriverConf(bool onlyFlags)
{
    ConfUtil confUtil;

    const bool ok = onlyFlags ? (confUtil.writeFlags(*conf()), true)
                              : confUtil.write(*conf(), this, *IoC<EnvManager>());

    if (!ok) {
        qCWarning(LC) << "Driver config error:" << confUtil.errorMessage();
        return false;
    }

    auto driverManager = IoC<DriverManager>();
    if (!driverManager->writeConf(confUtil.buffer(), onlyFlags)) {
        qCWarning(LC) << "Update driver error:" << driverManager->errorMessage();
        return false;
    }

    m_driveMask = confUtil.driveMask();

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
    app.useGroupPerm = stmt.columnBool(6);
    app.applyChild = stmt.columnBool(7);
    app.killChild = stmt.columnBool(8);
    app.lanOnly = stmt.columnBool(9);
    app.parked = stmt.columnBool(10);
    app.logBlocked = stmt.columnBool(11);
    app.logConn = stmt.columnBool(12);
    app.blocked = stmt.columnBool(13);
    app.killProcess = stmt.columnBool(14);
    app.acceptZones = stmt.columnUInt(15);
    app.rejectZones = stmt.columnUInt(16);
    app.ruleId = stmt.columnUInt(17);
    app.scheduleAction = stmt.columnInt(18);
    app.scheduleTime = stmt.columnDateTime(19);
    app.groupIndex = stmt.columnInt(20);
    app.alerted = stmt.columnBool(21);
}

bool ConfAppManager::updateDriverDeleteApp(const QString &appPath)
{
    App app;
    app.appPath = appPath;

    return updateDriverUpdateApp(app, /*remove=*/true);
}

bool ConfAppManager::updateDriverUpdateApp(const App &app, bool remove)
{
    ConfUtil confUtil;

    if (!confUtil.writeAppEntry(app)) {
        qCWarning(LC) << "Driver config error:" << confUtil.errorMessage();
        return false;
    }

    auto driverManager = IoC<DriverManager>();
    if (!driverManager->writeApp(confUtil.buffer(), remove)) {
        qCWarning(LC) << "Update driver error:" << driverManager->errorMessage();
        return false;
    }

    m_driveMask |= remove ? 0 : confUtil.driveMask();

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
