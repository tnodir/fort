#include "confappmanager.h"

#include <QLoggingCategory>

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
                                  "    LEFT JOIN app_alert alert ON alert.app_id = t.app_id;";

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
                                 "    end_action, end_time, creat_time)"
                                 "  VALUES(?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14,"
                                 "    ?15, ?16, ?17, ?18, ?19, ?20, ?21)"
                                 "  ON CONFLICT(path) DO UPDATE"
                                 "  SET app_group_id = ?2, origin_path = ?3, name = ?5,"
                                 "    notes = ?6, is_wildcard = ?7, use_group_perm = ?8,"
                                 "    apply_child = ?9, kill_child = ?10, lan_only = ?11,"
                                 "    parked = ?12, log_blocked = ?13, log_conn = ?14,"
                                 "    blocked = ?15, kill_process = ?16,"
                                 "    accept_zones = ?17, reject_zones = ?18,"
                                 "    end_action = ?19, end_time = ?20"
                                 "  RETURNING app_id;";

const char *const sqlUpdateApp = "UPDATE app"
                                 "  SET app_group_id = ?2, origin_path = ?3, path = ?4, name = ?5,"
                                 "    notes = ?6, is_wildcard = ?7, use_group_perm = ?8,"
                                 "    apply_child = ?9, kill_child = ?10, lan_only = ?11,"
                                 "    parked = ?12, log_blocked = ?13, log_conn = ?14,"
                                 "    blocked = ?15, kill_process = ?16,"
                                 "    accept_zones = ?17, reject_zones = ?18,"
                                 "    end_action = ?19, end_time = ?20"
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
    m_confManager = IoCPinned()->setUpDependency<ConfManager>();

    setupDriveListManager();

    purgeAppsOnStart();

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

void ConfAppManager::purgeAppsOnStart()
{
    if (conf()->ini().progPurgeOnStart()) {
        purgeApps();
    }
}

void ConfAppManager::setupAppEndTimer()
{
    auto logManager = IoC<LogManager>();

    connect(logManager, &LogManager::systemTimeChanged, this, &ConfAppManager::updateAppEndTimer);

    updateAppEndTimer();
}

void ConfAppManager::updateAppEndTimer()
{
    const qint64 endTimeMsecs = sqliteDb()->executeEx(sqlSelectMinEndApp).toLongLong();

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

void ConfAppManager::beginAddOrUpdateApp(
        App &app, const AppGroup &appGroup, bool onlyUpdate, bool &ok)
{
    const auto vars = QVariantList()
            << app.appId << appGroup.id() << app.appOriginPath << SqliteStmt::nullable(app.appPath)
            << app.appName << app.notes << app.isWildcard << app.useGroupPerm << app.applyChild
            << app.killChild << app.lanOnly << app.parked << app.logBlocked << app.logConn
            << app.blocked << app.killProcess << app.acceptZones << app.rejectZones
            << app.scheduleAction << SqliteStmt::nullable(app.scheduleTime)
            << SqliteStmt::nullable(DateUtil::now(), onlyUpdate);

    const char *sql = onlyUpdate ? sqlUpdateApp : sqlUpsertApp;
    const auto appIdVar = sqliteDb()->executeEx(sql, vars, 1, &ok);

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
    const QString appOriginPath = logEntry.path();
    const QString appPath = FileUtil::normalizePath(appOriginPath);

    if (appIdByPath(appPath) > 0)
        return; // already added by user

    const QString appName = IoC<AppInfoCache>()->appName(appOriginPath);

    App app;
    app.blocked = logEntry.blocked();
    app.alerted = true;
    app.groupIndex = 0; // "Main" app. group
    app.appOriginPath = appOriginPath;
    app.appPath = appPath;
    app.appName = appName;
    app.scheduleAction = App::ScheduleRemove; // default action for alert

    const bool ok = addOrUpdateApp(app);
    if (ok) {
        emitAppAlerted();
    }
}

qint64 ConfAppManager::appIdByPath(const QString &appPath)
{
    return sqliteDb()->executeEx(sqlSelectAppIdByPath, { appPath }).toLongLong();
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
        sqliteDb()->executeEx(sql, { app.appId });
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

    const auto vars = QVariantList() << appId << appName;

    sqliteDb()->executeEx(sqlUpdateAppName, vars, 0, &ok);

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

    const auto vars = QVariantList() << appId;

    const auto resList = sqliteDb()->executeEx(sqlDeleteApp, vars, 2, &ok).toList();

    if (ok) {
        sqliteDb()->executeEx(sqlDeleteAppAlert, vars, 0, &ok);
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
    if (!sqliteDb()->prepare(stmt, sqlSelectAppsToPurge))
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

bool ConfAppManager::walkApps(const std::function<walkAppsCallback> &func)
{
    SqliteStmt stmt;
    if (!sqliteDb()->prepare(stmt, sqlSelectApps))
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

    const auto vars = QVariantList() << app.appId << app.blocked << app.killProcess;

    sqliteDb()->executeEx(sqlUpdateAppBlocked, vars, 0, &ok);

    if (ok) {
        sqliteDb()->executeEx(sqlDeleteAppAlert, { app.appId }, 0, &ok);
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
            app.blocked = (app.scheduleAction == App::ScheduleBlock);
            app.killProcess = false;
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
    return sqliteDb()->executeEx(sqlSelectMaxAlertAppId).toLongLong();
}

bool ConfAppManager::updateDriverConf(bool onlyFlags)
{
    ConfUtil confUtil;
    QByteArray buf;

    const int confSize = onlyFlags ? confUtil.writeFlags(*conf(), buf)
                                   : confUtil.write(*conf(), this, *IoC<EnvManager>(), buf);

    if (confSize == 0) {
        qCWarning(LC) << "Driver config error:" << confUtil.errorMessage();
        return false;
    }

    auto driverManager = IoC<DriverManager>();
    if (!driverManager->writeConf(buf, confSize, onlyFlags)) {
        qCWarning(LC) << "Update driver error:" << driverManager->errorMessage();
        return false;
    }

    m_driveMask = confUtil.driveMask();

    return true;
}

bool ConfAppManager::loadAppById(App &app)
{
    SqliteStmt stmt;
    if (!sqliteDb()->prepare(stmt, sqlSelectAppById))
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
    app.scheduleAction = stmt.columnInt(17);
    app.scheduleTime = stmt.columnDateTime(18);
    app.groupIndex = stmt.columnInt(19);
    app.alerted = stmt.columnBool(20);
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
    QByteArray buf;

    const int entrySize = confUtil.writeAppEntry(app, /*isNew=*/false, buf);

    if (entrySize == 0) {
        qCWarning(LC) << "Driver config error:" << confUtil.errorMessage();
        return false;
    }

    auto driverManager = IoC<DriverManager>();
    if (!driverManager->writeApp(buf, entrySize, remove)) {
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
    return sqliteDb()->beginTransaction();
}

void ConfAppManager::commitTransaction(bool &ok)
{
    ok = sqliteDb()->endTransaction(ok);
}
