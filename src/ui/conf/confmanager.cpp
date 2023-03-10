#include "confmanager.h"

#include <QLoggingCategory>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <appinfo/appinfocache.h>
#include <appinfo/appinfoutil.h>
#include <conf/app.h>
#include <conf/zone.h>
#include <driver/drivercommon.h>
#include <driver/drivermanager.h>
#include <fortsettings.h>
#include <log/logentryblocked.h>
#include <log/logmanager.h>
#include <manager/envmanager.h>
#include <manager/windowmanager.h>
#include <task/taskinfo.h>
#include <task/taskmanager.h>
#include <user/iniuser.h>
#include <user/usersettings.h>
#include <util/conf/confutil.h>
#include <util/dateutil.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>
#include <util/startuputil.h>

#include "addressgroup.h"
#include "appgroup.h"
#include "firewallconf.h"

namespace {

const QLoggingCategory LC("conf");

constexpr int DATABASE_USER_VERSION = 18;

constexpr int APP_END_TIMER_INTERVAL_MIN = 100;
constexpr int APP_END_TIMER_INTERVAL_MAX = 24 * 60 * 60 * 1000; // 1 day

const char *const sqlSelectAddressGroups = "SELECT addr_group_id, include_all, exclude_all,"
                                           "    include_zones, exclude_zones,"
                                           "    include_text, exclude_text"
                                           "  FROM address_group"
                                           "  ORDER BY order_index;";

const char *const sqlSelectAppGroups = "SELECT app_group_id, enabled, apply_child,"
                                       "    lan_only, log_blocked, log_conn, period_enabled,"
                                       "    limit_in_enabled, limit_out_enabled,"
                                       "    speed_limit_in, speed_limit_out,"
                                       "    limit_packet_loss, limit_latency,"
                                       "    limit_bufsize_in, limit_bufsize_out,"
                                       "    name, block_text, allow_text,"
                                       "    period_from, period_to"
                                       "  FROM app_group"
                                       "  ORDER BY order_index;";

const char *const sqlInsertAddressGroup = "INSERT INTO address_group(addr_group_id, order_index,"
                                          "    include_all, exclude_all,"
                                          "    include_zones, exclude_zones,"
                                          "    include_text, exclude_text)"
                                          "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8);";

const char *const sqlUpdateAddressGroup = "UPDATE address_group"
                                          "  SET order_index = ?2,"
                                          "    include_all = ?3, exclude_all = ?4,"
                                          "    include_zones = ?5, exclude_zones = ?6,"
                                          "    include_text = ?7, exclude_text = ?8"
                                          "  WHERE addr_group_id = ?1;";

const char *const sqlInsertAppGroup = "INSERT INTO app_group(app_group_id, order_index, enabled,"
                                      "    apply_child, lan_only, log_blocked, log_conn,"
                                      "    period_enabled, limit_in_enabled, limit_out_enabled,"
                                      "    speed_limit_in, speed_limit_out,"
                                      "    limit_packet_loss, limit_latency,"
                                      "    limit_bufsize_in, limit_bufsize_out,"
                                      "    name, block_text, allow_text,"
                                      "    period_from, period_to)"
                                      "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12,"
                                      "    ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20, ?21);";

const char *const sqlUpdateAppGroup = "UPDATE app_group"
                                      "  SET order_index = ?2, enabled = ?3,"
                                      "    apply_child = ?4, lan_only = ?5,"
                                      "    log_blocked = ?6, log_conn = ?7, period_enabled = ?8,"
                                      "    limit_in_enabled = ?9, limit_out_enabled = ?10,"
                                      "    speed_limit_in = ?11, speed_limit_out = ?12,"
                                      "    limit_packet_loss = ?13, limit_latency = ?14,"
                                      "    limit_bufsize_in = ?15, limit_bufsize_out = ?16,"
                                      "    name = ?17, block_text = ?18, allow_text = ?19,"
                                      "    period_from = ?20, period_to = ?21"
                                      "  WHERE app_group_id = ?1;";

const char *const sqlDeleteAppGroup = "DELETE FROM app_group"
                                      "  WHERE app_group_id = ?1;";

const char *const sqlSelectTaskByName = "SELECT task_id, enabled, interval_hours,"
                                        "    last_run, last_success, data"
                                        "  FROM task"
                                        "  WHERE name = ?1;";

const char *const sqlInsertTask = "INSERT INTO task(task_id, name, enabled, interval_hours,"
                                  "    last_run, last_success, data)"
                                  "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7);";

const char *const sqlUpdateTask = "UPDATE task"
                                  "  SET name = ?2, enabled = ?3, interval_hours = ?4,"
                                  "    last_run = ?5, last_success = ?6, data = ?7"
                                  "  WHERE task_id = ?1;";

const char *const sqlSelectAppPaths = "SELECT app_id, path FROM app;";

const char *const sqlSelectAppById = "SELECT"
                                     "    g.order_index as group_index,"
                                     "    t.path,"
                                     "    t.use_group_perm,"
                                     "    t.apply_child,"
                                     "    t.lan_only,"
                                     "    t.log_blocked,"
                                     "    t.log_conn,"
                                     "    t.blocked"
                                     "  FROM app t"
                                     "    JOIN app_group g ON g.app_group_id = t.app_group_id"
                                     "    WHERE app_id = ?1;";

const char *const sqlSelectApps = "SELECT"
                                  "    g.order_index as group_index,"
                                  "    t.path,"
                                  "    t.use_group_perm,"
                                  "    t.apply_child,"
                                  "    t.lan_only,"
                                  "    t.log_blocked,"
                                  "    t.log_conn,"
                                  "    t.blocked,"
                                  "    (alert.app_id IS NOT NULL) as alerted"
                                  "  FROM app t"
                                  "    JOIN app_group g ON g.app_group_id = t.app_group_id"
                                  "    LEFT JOIN app_alert alert ON alert.app_id = t.app_id;";

const char *const sqlSelectMinEndApp = "SELECT MIN(end_time) FROM app"
                                       "  WHERE end_time != 0 AND blocked = 0;";

const char *const sqlSelectEndedApps = "SELECT t.app_id, g.order_index as group_index,"
                                       "    t.path, t.name, t.use_group_perm,"
                                       "    t.apply_child, t.lan_only, t.log_blocked, t.log_conn"
                                       "  FROM app t"
                                       "    JOIN app_group g ON g.app_group_id = t.app_group_id"
                                       "  WHERE end_time <= ?1 AND blocked = 0;";

const char *const sqlSelectAppIdByPath = "SELECT app_id FROM app WHERE path = ?1;";

const char *const sqlUpsertApp = "INSERT INTO app(app_group_id, path, name,"
                                 "    use_group_perm, apply_child, lan_only,"
                                 "    log_blocked, log_conn, blocked,"
                                 "    creat_time, end_time)"
                                 "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11)"
                                 "  ON CONFLICT(path) DO UPDATE"
                                 "  SET app_group_id = ?1, name = ?3,"
                                 "    use_group_perm = ?4, apply_child = ?5,"
                                 "    lan_only = ?6, log_blocked = ?7, log_conn = ?8,"
                                 "    blocked = ?9, creat_time = ?10, end_time = ?11"
                                 "  RETURNING app_id;";

const char *const sqlInsertAppAlert = "INSERT INTO app_alert(app_id) VALUES(?1);";

const char *const sqlDeleteApp = "DELETE FROM app WHERE app_id = ?1 RETURNING path;";

const char *const sqlDeleteAppAlert = "DELETE FROM app_alert WHERE app_id = ?1;";

const char *const sqlUpdateApp = "UPDATE app"
                                 "  SET app_group_id = ?2, name = ?3, use_group_perm = ?4,"
                                 "    apply_child = ?5, lan_only = ?6,"
                                 "    log_blocked = ?7, log_conn = ?8,"
                                 "    blocked = ?9, end_time = ?10"
                                 "  WHERE app_id = ?1;";

const char *const sqlUpdateAppName = "UPDATE app SET name = ?2 WHERE app_id = ?1;";

const char *const sqlUpdateAppBlocked = "UPDATE app SET blocked = ?2, end_time = NULL"
                                        "  WHERE app_id = ?1;";

const char *const sqlUpdateAppResetGroup = "UPDATE app"
                                           "  SET app_group_id = ?2"
                                           "  WHERE app_group_id = ?1;";

const char *const sqlInsertZone = "INSERT INTO zone(zone_id, name, enabled, custom_url,"
                                  "    source_code, url, form_data)"
                                  "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7);";

const char *const sqlSelectZoneIds = "SELECT zone_id FROM zone ORDER BY zone_id;";

const char *const sqlDeleteZone = "DELETE FROM zone WHERE zone_id = ?1;";

const char *const sqlDeleteAddressGroupZone = "UPDATE address_group"
                                              "  SET include_zones = include_zones & ?1,"
                                              "    exclude_zones = exclude_zones & ?1;";

const char *const sqlUpdateZone = "UPDATE zone"
                                  "  SET name = ?2, enabled = ?3, custom_url = ?4,"
                                  "    source_code = ?5, url = ?6, form_data = ?7"
                                  "  WHERE zone_id = ?1;";

const char *const sqlUpdateZoneName = "UPDATE zone SET name = ?2 WHERE zone_id = ?1;";

const char *const sqlUpdateZoneEnabled = "UPDATE zone SET enabled = ?2 WHERE zone_id = ?1;";

const char *const sqlUpdateZoneResult =
        "UPDATE zone"
        "  SET address_count = ?2, text_checksum = ?3, bin_checksum = ?4,"
        "    source_modtime = ?5, last_run = ?6, last_success = ?7"
        "  WHERE zone_id = ?1;";

bool migrateFunc(SqliteDb *db, int version, bool isNewDb, void *ctx)
{
    Q_UNUSED(ctx);

    if (isNewDb)
        return true;

    switch (version) {
    case 6: {
        // COMPAT: Zones
        db->execute("UPDATE task SET name = 'ZoneDownloader' WHERE name = 'Tasix';");
    } break;
    }

    return true;
}

bool loadAddressGroups(SqliteDb *db, const QList<AddressGroup *> &addressGroups, int &index)
{
    SqliteStmt stmt;
    if (!db->prepare(stmt, sqlSelectAddressGroups))
        return false;

    index = 0;
    while (stmt.step() == SqliteStmt::StepRow) {
        auto addrGroup = addressGroups.at(index);
        Q_ASSERT(addrGroup);

        addrGroup->setId(stmt.columnInt64(0));
        addrGroup->setIncludeAll(stmt.columnBool(1));
        addrGroup->setExcludeAll(stmt.columnBool(2));
        addrGroup->setIncludeZones(quint32(stmt.columnInt64(3)));
        addrGroup->setExcludeZones(quint32(stmt.columnInt64(4)));
        addrGroup->setIncludeText(stmt.columnText(5));
        addrGroup->setExcludeText(stmt.columnText(6));

        addrGroup->setEdited(false);

        if (++index > 1)
            break;
    }

    return true;
}

bool saveAddressGroup(SqliteDb *db, AddressGroup *addrGroup, int orderIndex)
{
    const bool rowExists = (addrGroup->id() != 0);
    if (!addrGroup->edited() && rowExists)
        return true;

    const auto vars = QVariantList()
            << (rowExists ? addrGroup->id() : QVariant()) << orderIndex << addrGroup->includeAll()
            << addrGroup->excludeAll() << qint64(addrGroup->includeZones())
            << qint64(addrGroup->excludeZones()) << addrGroup->includeText()
            << addrGroup->excludeText();

    const char *sql = rowExists ? sqlUpdateAddressGroup : sqlInsertAddressGroup;

    bool ok;
    db->executeEx(sql, vars, 0, &ok);
    if (!ok)
        return false;

    if (!rowExists) {
        addrGroup->setId(db->lastInsertRowid());
    }

    addrGroup->setEdited(false);

    return true;
}

bool saveAddressGroups(SqliteDb *db, const FirewallConf &conf)
{
    int orderIndex = 0;
    for (AddressGroup *addrGroup : conf.addressGroups()) {
        if (!saveAddressGroup(db, addrGroup, orderIndex++))
            return false;
    }
    return true;
}

bool loadAppGroups(SqliteDb *db, FirewallConf &conf)
{
    SqliteStmt stmt;
    if (!db->prepare(stmt, sqlSelectAppGroups))
        return false;

    while (stmt.step() == SqliteStmt::StepRow) {
        auto appGroup = new AppGroup();

        appGroup->setId(stmt.columnInt64(0));
        appGroup->setEnabled(stmt.columnBool(1));
        appGroup->setApplyChild(stmt.columnBool(2));
        appGroup->setLanOnly(stmt.columnBool(3));
        appGroup->setLogBlocked(stmt.columnBool(4));
        appGroup->setLogConn(stmt.columnBool(5));
        appGroup->setPeriodEnabled(stmt.columnBool(6));
        appGroup->setLimitInEnabled(stmt.columnBool(7));
        appGroup->setLimitOutEnabled(stmt.columnBool(8));
        appGroup->setSpeedLimitIn(quint32(stmt.columnInt(9)));
        appGroup->setSpeedLimitOut(quint32(stmt.columnInt(10)));
        appGroup->setLimitPacketLoss(quint16(stmt.columnInt(11)));
        appGroup->setLimitLatency(quint32(stmt.columnInt(12)));
        appGroup->setLimitBufferSizeIn(quint32(stmt.columnInt(13)));
        appGroup->setLimitBufferSizeOut(quint32(stmt.columnInt(14)));
        appGroup->setName(stmt.columnText(15));
        appGroup->setBlockText(stmt.columnText(16));
        appGroup->setAllowText(stmt.columnText(17));
        appGroup->setPeriodFrom(stmt.columnText(18));
        appGroup->setPeriodTo(stmt.columnText(19));
        appGroup->setEdited(false);

        conf.addAppGroup(appGroup);
    }

    return true;
}

bool saveAppGroup(SqliteDb *db, AppGroup *appGroup, int orderIndex)
{
    const bool rowExists = (appGroup->id() != 0);
    if (!appGroup->edited() && rowExists)
        return true;

    const auto vars = QVariantList()
            << (rowExists ? appGroup->id() : QVariant()) << orderIndex << appGroup->enabled()
            << appGroup->applyChild() << appGroup->lanOnly() << appGroup->logBlocked()
            << appGroup->logConn() << appGroup->periodEnabled() << appGroup->limitInEnabled()
            << appGroup->limitOutEnabled() << appGroup->speedLimitIn() << appGroup->speedLimitOut()
            << appGroup->limitPacketLoss() << appGroup->limitLatency()
            << appGroup->limitBufferSizeIn() << appGroup->limitBufferSizeOut() << appGroup->name()
            << appGroup->blockText() << appGroup->allowText() << appGroup->periodFrom()
            << appGroup->periodTo();

    const char *sql = rowExists ? sqlUpdateAppGroup : sqlInsertAppGroup;

    bool ok;
    db->executeEx(sql, vars, 0, &ok);
    if (!ok)
        return false;

    if (!rowExists) {
        appGroup->setId(db->lastInsertRowid());
    }
    appGroup->setEdited(false);

    return true;
}

bool saveAppGroups(SqliteDb *db, const FirewallConf &conf)
{
    int orderIndex = 0;
    for (AppGroup *appGroup : conf.appGroups()) {
        if (!saveAppGroup(db, appGroup, orderIndex++))
            return false;
    }
    return true;
}

bool removeAppGroupsInDb(SqliteDb *db, const FirewallConf &conf)
{
    Q_ASSERT(!conf.appGroups().isEmpty());
    const auto defaultAppGroupId = conf.appGroups().at(0)->id();

    for (const qint64 appGroupId : conf.removedAppGroupIdList()) {
        bool ok;

        db->executeEx(sqlUpdateAppResetGroup, { appGroupId, defaultAppGroupId }, 0);

        db->executeEx(sqlDeleteAppGroup, { appGroupId }, 0, &ok);
        if (!ok)
            return false;
    }

    conf.clearRemovedAppGroupIdList();

    return true;
}

}

ConfManager::ConfManager(const QString &filePath, QObject *parent, quint32 openFlags) :
    QObject(parent), m_sqliteDb(new SqliteDb(filePath, openFlags)), m_conf(createConf())
{
    connect(&m_appAlertedTimer, &QTimer::timeout, this, &ConfManager::appAlerted);
    connect(&m_appChangedTimer, &QTimer::timeout, this, &ConfManager::appChanged);
    connect(&m_appUpdatedTimer, &QTimer::timeout, this, &ConfManager::appUpdated);

    m_appEndTimer.setSingleShot(true);
    connect(&m_appEndTimer, &QTimer::timeout, this, &ConfManager::updateAppEndTimes);
}

IniUser *ConfManager::iniUser() const
{
    return &IoC<UserSettings>()->iniUser();
}

void ConfManager::showErrorMessage(const QString &errorMessage)
{
    IoC<WindowManager>()->showErrorBox(errorMessage, tr("Configuration Error"));
}

void ConfManager::setUp()
{
    if (!sqliteDb()->open()) {
        qCCritical(LC) << "File open error:" << sqliteDb()->filePath()
                       << sqliteDb()->errorMessage();
        return;
    }

    SqliteDb::MigrateOptions opt = { .sqlDir = ":/conf/migrations",
        .version = DATABASE_USER_VERSION,
        .recreate = true,
        .migrateFunc = &migrateFunc };

    if (!sqliteDb()->migrate(opt)) {
        qCCritical(LC) << "Migration error" << sqliteDb()->filePath();
        return;
    }

    setupAppEndTimer();
}

void ConfManager::setupAppEndTimer()
{
    auto logManager = IoC<LogManager>();

    connect(logManager, &LogManager::systemTimeChanged, this, &ConfManager::updateAppEndTimer);

    updateAppEndTimer();
}

void ConfManager::updateAppEndTimer()
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

void ConfManager::initConfToEdit()
{
    if (confToEdit())
        return;

    auto newConf = createConf();
    newConf->copy(*conf());

    loadExtFlags(newConf->ini());

    setConfToEdit(newConf);
}

void ConfManager::setConfToEdit(FirewallConf *conf)
{
    if (confToEdit() == conf)
        return;

    if (confToEdit() && confToEdit() != this->conf()) {
        confToEdit()->deleteLater();
    }

    m_confToEdit = conf;
}

void ConfManager::initIniUserToEdit()
{
    if (iniUserToEdit())
        return;

    auto newIniUser = new IniUser(iniUser()->settings());

    setIniUserToEdit(newIniUser);
}

void ConfManager::setIniUserToEdit(IniUser *iniUser)
{
    if (iniUserToEdit() == iniUser)
        return;

    if (iniUserToEdit() && iniUserToEdit() != this->iniUser()) {
        delete m_iniUserToEdit;
    }

    m_iniUserToEdit = iniUser;
}

void ConfManager::setConf(FirewallConf *newConf)
{
    conf()->deleteLater();
    m_conf = newConf;

    if (confToEdit() == conf()) {
        setConfToEdit(nullptr);
    }
}

FirewallConf *ConfManager::createConf()
{
    FirewallConf *conf = new FirewallConf(IoC<FortSettings>(), this);
    return conf;
}

void ConfManager::setupDefault(FirewallConf &conf) const
{
    conf.setupDefaultAddressGroups();
    conf.addDefaultAppGroup();
}

void ConfManager::emitAppAlerted()
{
    m_appAlertedTimer.startTrigger();
}

void ConfManager::emitAppChanged()
{
    m_appChangedTimer.startTrigger();
}

void ConfManager::emitAppUpdated()
{
    m_appUpdatedTimer.startTrigger();
}

bool ConfManager::loadConf(FirewallConf &conf)
{
    if (conf.optEdited()) {
        bool isNewConf = false;

        if (!loadFromDb(conf, isNewConf)) {
            showErrorMessage("Load Settings: " + sqliteDb()->errorMessage());
            return false;
        }

        if (isNewConf) {
            setupDefault(conf);
            saveToDb(conf);
        }
    }

    IoC<FortSettings>()->readConfIni(conf);

    return true;
}

bool ConfManager::load()
{
    Q_ASSERT(conf());

    if (!loadConf(*conf()))
        return false;

    applySavedConf(conf());

    return true;
}

bool ConfManager::saveConf(FirewallConf &conf)
{
    conf.prepareToSave();

    if (conf.optEdited() && !saveToDb(conf))
        return false;

    IoC<FortSettings>()->writeConfIni(conf);

    if (conf.iniEdited()) {
        saveExtFlags(conf.ini());
    }

    if (conf.taskEdited()) {
        saveTasksByIni(conf.ini());
    }

    conf.afterSaved();

    return true;
}

void ConfManager::applySavedConf(FirewallConf *newConf)
{
    if (!newConf->anyEdited())
        return;

    const bool onlyFlags = !newConf->optEdited();

    if (conf() != newConf) {
        if (onlyFlags) {
            conf()->copyFlags(*newConf);
            newConf->resetEdited();
        } else {
            setConf(newConf);
        }
    }

    emit confChanged(onlyFlags);

    if (conf()->iniEdited()) {
        emit iniChanged(conf()->ini());
    }

    conf()->resetEdited();
}

bool ConfManager::save(FirewallConf *newConf)
{
    if (!newConf->anyEdited())
        return true;

    if (!(validateConf(*newConf) && saveConf(*newConf)))
        return false;

    applySavedConf(newConf);

    return true;
}

bool ConfManager::saveFlags()
{
    conf()->setFlagsEdited();
    return save(conf());
}

void ConfManager::saveIni()
{
    conf()->setIniEdited();

    saveConf(*conf());

    conf()->resetEdited();
}

void ConfManager::saveIniUser(bool flagsChanged)
{
    iniUser()->save();
    iniUser()->clear();

    if (flagsChanged) {
        emit iniUserChanged(/*onlyFlags=*/true);
    }
}

QVariant ConfManager::toPatchVariant(bool onlyFlags) const
{
    return onlyFlags ? conf()->toVariant(true) // send only flags to clients
                     : FirewallConf::editedFlagsToVariant(
                             FirewallConf::AllEdited); // clients have to reload all from storage
}

bool ConfManager::saveVariant(const QVariant &confVar)
{
    const uint editedFlags = FirewallConf::editedFlagsFromVariant(confVar);

    FirewallConf *conf;
    bool isNewConf = false;

    if ((editedFlags & FirewallConf::OptEdited) != 0) {
        conf = createConf();
        conf->copyFlags(*this->conf());
        isNewConf = true;
    } else {
        conf = this->conf();
    }

    conf->fromVariant(confVar, true);

    if (!saveConf(*conf)) {
        if (isNewConf) {
            delete conf;
        }
        return false;
    }

    applySavedConf(conf);

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

    sqliteDb()->beginTransaction();

    for (TaskInfo *taskInfo : taskInfos) {
        if (!saveTask(taskInfo)) {
            ok = false;
            break;
        }
    }

    return checkResult(ok, true);
}

void ConfManager::logBlockedApp(const LogEntryBlocked &logEntry)
{
    const QString appPath = logEntry.path();

    if (appIdByPath(appPath) > 0)
        return; // already added by user

    const QString appName = IoC<AppInfoCache>()->appName(appPath);

    App app;
    app.blocked = logEntry.blocked();
    app.alerted = true;
    app.groupIndex = 0; // "Main" app. group
    app.appPath = appPath;
    app.appName = appName;

    const bool ok = addOrUpdateApp(app);
    if (ok) {
        emitAppAlerted();
    }
}

qint64 ConfManager::appIdByPath(const QString &appPath)
{
    return sqliteDb()->executeEx(sqlSelectAppIdByPath, { appPath }).toLongLong();
}

bool ConfManager::addApp(const App &app)
{
    if (!updateDriverUpdateApp(app, /*remove=*/false))
        return false;

    return addOrUpdateApp(app);
}

bool ConfManager::deleteApp(qint64 appId)
{
    bool ok = false;

    sqliteDb()->beginTransaction();

    const auto vars = QVariantList() << appId;

    const QString appPath = sqliteDb()->executeEx(sqlDeleteApp, vars, 1, &ok).toString();
    if (ok) {
        sqliteDb()->executeEx(sqlDeleteAppAlert, vars, 0, &ok);
    }

    checkResult(ok, true);

    if (ok) {
        updateDriverDeleteApp(appPath);

        emitAppChanged();
    }

    return ok;
}

bool ConfManager::purgeApps()
{
    QVector<qint64> appIdList;

    // Collect non-existent apps
    {
        SqliteStmt stmt;
        if (!sqliteDb()->prepare(stmt, sqlSelectAppPaths))
            return false;

        while (stmt.step() == SqliteStmt::StepRow) {
            const QString appPath = stmt.columnText(1);

            if (FileUtil::isDriveFilePath(appPath) && !AppInfoUtil::fileExists(appPath)) {
                const qint64 appId = stmt.columnInt64(0);
                appIdList.append(appId);
            }
        }
    }

    // Delete apps
    for (const qint64 appId : appIdList) {
        deleteApp(appId);
    }

    return true;
}

bool ConfManager::updateApp(const App &app)
{
    const AppGroup *appGroup = conf()->appGroupAt(app.groupIndex);
    if (appGroup->isNull())
        return false;

    if (!updateDriverUpdateApp(app))
        return false;

    bool ok = false;

    sqliteDb()->beginTransaction();

    const auto vars = QVariantList()
            << app.appId << appGroup->id() << app.appName << app.useGroupPerm << app.applyChild
            << app.lanOnly << app.logBlocked << app.logConn << app.blocked
            << (!app.endTime.isNull() ? app.endTime : QVariant());

    sqliteDb()->executeEx(sqlUpdateApp, vars, 0, &ok);
    if (ok) {
        sqliteDb()->executeEx(sqlDeleteAppAlert, { app.appId }, 0, &ok);
    }

    checkResult(ok, true);

    if (ok) {
        if (!app.endTime.isNull()) {
            updateAppEndTimer();
        }

        emitAppUpdated();
    }

    return ok;
}

bool ConfManager::updateAppBlocked(qint64 appId, bool blocked)
{
    bool changed = false;
    if (!updateDriverAppBlocked(appId, blocked, changed))
        return false;

    bool ok = true;

    sqliteDb()->beginTransaction();

    const auto vars = QVariantList() << appId << blocked;

    if (changed) {
        sqliteDb()->executeEx(sqlUpdateAppBlocked, vars, 0, &ok);
    }
    if (ok) {
        sqliteDb()->executeEx(sqlDeleteAppAlert, { appId }, 0, &ok);
    }

    checkResult(ok, true);

    if (ok) {
        emitAppUpdated();
    }

    return ok;
}

bool ConfManager::updateAppName(qint64 appId, const QString &appName)
{
    bool ok = false;

    const auto vars = QVariantList() << appId << appName;

    sqliteDb()->executeEx(sqlUpdateAppName, vars, 0, &ok);

    checkResult(ok);

    if (ok) {
        emitAppUpdated();
    }

    return ok;
}

bool ConfManager::walkApps(const std::function<walkAppsCallback> &func)
{
    SqliteStmt stmt;
    if (!sqliteDb()->prepare(stmt, sqlSelectApps))
        return false;

    while (stmt.step() == SqliteStmt::StepRow) {
        App app;
        app.groupIndex = stmt.columnInt(0);
        app.appPath = stmt.columnText(1);
        app.useGroupPerm = stmt.columnBool(2);
        app.applyChild = stmt.columnBool(3);
        app.lanOnly = stmt.columnBool(4);
        app.logBlocked = stmt.columnBool(5);
        app.logConn = stmt.columnBool(6);
        app.blocked = stmt.columnBool(7);
        app.alerted = stmt.columnBool(8);

        if (!func(app))
            return false;
    }

    return true;
}

void ConfManager::updateAppEndTimes()
{
    SqliteStmt stmt;
    if (!stmt.prepare(sqliteDb()->db(), sqlSelectEndedApps))
        return;

    stmt.bindDateTime(1, QDateTime::currentDateTime());

    while (stmt.step() == SqliteStmt::StepRow) {
        App app;
        app.appId = stmt.columnInt64(0);
        app.groupIndex = stmt.columnInt(1);
        app.appPath = stmt.columnText(2);
        app.appName = stmt.columnText(3);
        app.useGroupPerm = stmt.columnBool(4);
        app.applyChild = stmt.columnBool(5);
        app.lanOnly = stmt.columnBool(6);
        app.logBlocked = stmt.columnBool(7);
        app.logConn = stmt.columnBool(8);
        app.blocked = true;

        updateApp(app);
    }

    updateAppEndTimer();
}

bool ConfManager::addZone(Zone &zone)
{
    bool ok = false;

    zone.zoneId = getFreeZoneId();

    const auto vars = QVariantList()
            << zone.zoneId << zone.zoneName << zone.enabled << zone.customUrl << zone.sourceCode
            << zone.url << zone.formData;

    sqliteDb()->executeEx(sqlInsertZone, vars, 0, &ok);

    checkResult(ok);

    if (ok) {
        emit zoneAdded();
    }

    return ok;
}

int ConfManager::getFreeZoneId()
{
    int zoneId = 1;

    SqliteStmt stmt;
    if (stmt.prepare(sqliteDb()->db(), sqlSelectZoneIds)) {
        while (stmt.step() == SqliteStmt::StepRow) {
            const int id = stmt.columnInt(0);
            if (id > zoneId)
                break;

            zoneId = id + 1;
        }
    }

    return zoneId;
}

bool ConfManager::deleteZone(int zoneId)
{
    bool ok = false;

    sqliteDb()->beginTransaction();

    sqliteDb()->executeEx(sqlDeleteZone, { zoneId }, 0, &ok);
    if (ok) {
        // Delete the Zone from Address Groups
        const quint32 zoneUnMask = ~(quint32(1) << (zoneId - 1));
        sqliteDb()->executeEx(sqlDeleteAddressGroupZone, { qint64(zoneUnMask) }, 0, &ok);
    }

    checkResult(ok, true);

    if (ok) {
        emit zoneRemoved(zoneId);
    }

    return ok;
}

bool ConfManager::updateZone(const Zone &zone)
{
    if (!updateDriverZoneFlag(zone.zoneId, zone.enabled))
        return false;

    bool ok = false;

    const auto vars = QVariantList()
            << zone.zoneId << zone.zoneName << zone.enabled << zone.customUrl << zone.sourceCode
            << zone.url << zone.formData;

    sqliteDb()->executeEx(sqlUpdateZone, vars, 0, &ok);

    checkResult(ok);

    if (ok) {
        emit zoneUpdated();
    }

    return ok;
}

bool ConfManager::updateZoneName(int zoneId, const QString &zoneName)
{
    bool ok = false;

    const auto vars = QVariantList() << zoneId << zoneName;

    sqliteDb()->executeEx(sqlUpdateZoneName, vars, 0, &ok);

    checkResult(ok);

    if (ok) {
        emit zoneUpdated();
    }

    return ok;
}

bool ConfManager::updateZoneEnabled(int zoneId, bool enabled)
{
    if (!updateDriverZoneFlag(zoneId, enabled))
        return false;

    bool ok = false;

    const auto vars = QVariantList() << zoneId << enabled;

    sqliteDb()->executeEx(sqlUpdateZoneEnabled, vars, 0, &ok);

    checkResult(ok);

    if (ok) {
        emit zoneUpdated();
    }

    return ok;
}

bool ConfManager::updateZoneResult(const Zone &zone)
{
    bool ok = false;

    const auto vars = QVariantList()
            << zone.zoneId << zone.addressCount << zone.textChecksum << zone.binChecksum
            << zone.sourceModTime << zone.lastRun << zone.lastSuccess;

    sqliteDb()->executeEx(sqlUpdateZoneResult, vars, 0, &ok);

    checkResult(ok);

    if (ok) {
        emit zoneUpdated();
    }

    return ok;
}

bool ConfManager::checkPassword(const QString &password)
{
    return IoC<FortSettings>()->checkPassword(password);
}

bool ConfManager::validateConf(const FirewallConf &newConf)
{
    if (!newConf.optEdited())
        return true;

    ConfUtil confUtil;
    QByteArray buf;

    const int confSize = confUtil.write(newConf, this, *IoC<EnvManager>(), buf);
    if (confSize == 0) {
        showErrorMessage(confUtil.errorMessage());
        return false;
    }

    return true;
}

bool ConfManager::validateDriver()
{
    ConfUtil confUtil;
    QByteArray buf;

    const int verSize = confUtil.writeVersion(buf);

    auto driverManager = IoC<DriverManager>();
    return driverManager->validate(buf, verSize);
}

bool ConfManager::updateDriverConf(bool onlyFlags)
{
    ConfUtil confUtil;
    QByteArray buf;

    const int confSize = onlyFlags ? confUtil.writeFlags(*conf(), buf)
                                   : confUtil.write(*conf(), this, *IoC<EnvManager>(), buf);
    if (confSize == 0) {
        showErrorMessage(confUtil.errorMessage());
        return false;
    }

    auto driverManager = IoC<DriverManager>();
    if (!driverManager->writeConf(buf, confSize, onlyFlags)) {
        showErrorMessage(driverManager->errorMessage());
        return false;
    }

    return true;
}

bool ConfManager::addOrUpdateApp(const App &app)
{
    const AppGroup *appGroup = conf()->appGroupAt(app.groupIndex);
    if (appGroup->isNull())
        return false;

    bool ok = false;

    sqliteDb()->beginTransaction();

    const auto vars = QVariantList()
            << appGroup->id() << app.appPath << app.appName << app.useGroupPerm << app.applyChild
            << app.lanOnly << app.logBlocked << app.logConn << app.blocked
            << QDateTime::currentDateTime() << (!app.endTime.isNull() ? app.endTime : QVariant());

    const auto appIdVar = sqliteDb()->executeEx(sqlUpsertApp, vars, 1, &ok);

    if (ok) {
        // Alert
        const qint64 appId = appIdVar.toLongLong();
        sqliteDb()->executeEx(app.alerted ? sqlInsertAppAlert : sqlDeleteAppAlert, { appId });
    }

    checkResult(ok, true);

    if (ok) {
        if (!app.endTime.isNull()) {
            updateAppEndTimer();
        }

        emitAppChanged();
    }

    return ok;
}

bool ConfManager::updateDriverAppBlocked(qint64 appId, bool blocked, bool &changed)
{
    SqliteStmt stmt;
    if (!sqliteDb()->prepare(stmt, sqlSelectAppById))
        return false;

    stmt.bindInt64(1, appId);
    if (stmt.step() != SqliteStmt::StepRow)
        return false;

    App app;
    app.groupIndex = stmt.columnInt(0);
    app.appPath = stmt.columnText(1);
    app.useGroupPerm = stmt.columnBool(2);
    app.applyChild = stmt.columnBool(3);
    app.lanOnly = stmt.columnBool(4);
    app.logBlocked = stmt.columnBool(5);
    app.logConn = stmt.columnBool(6);
    app.blocked = stmt.columnBool(7);

    if (blocked != app.blocked) {
        app.blocked = blocked;

        if (!updateDriverUpdateApp(app))
            return false;

        changed = true;
    }

    return true;
}

bool ConfManager::updateDriverDeleteApp(const QString &appPath)
{
    App app;
    app.appPath = appPath;

    return updateDriverUpdateApp(app, /*remove=*/true);
}

bool ConfManager::updateDriverUpdateApp(const App &app, bool remove)
{
    ConfUtil confUtil;
    QByteArray buf;

    const int entrySize = confUtil.writeAppEntry(app, /*isNew=*/false, buf);

    if (entrySize == 0) {
        showErrorMessage(confUtil.errorMessage());
        return false;
    }

    auto driverManager = IoC<DriverManager>();
    if (!driverManager->writeApp(buf, entrySize, remove)) {
        showErrorMessage(driverManager->errorMessage());
        return false;
    }

    return true;
}

void ConfManager::updateDriverZones(quint32 zonesMask, quint32 enabledMask, quint32 dataSize,
        const QList<QByteArray> &zonesData)
{
    ConfUtil confUtil;
    QByteArray buf;

    const int entrySize = confUtil.writeZones(zonesMask, enabledMask, dataSize, zonesData, buf);

    if (entrySize == 0) {
        showErrorMessage(confUtil.errorMessage());
        return;
    }

    auto driverManager = IoC<DriverManager>();
    if (!driverManager->writeZones(buf, entrySize)) {
        showErrorMessage(driverManager->errorMessage());
        return;
    }
}

bool ConfManager::updateDriverZoneFlag(int zoneId, bool enabled)
{
    ConfUtil confUtil;
    QByteArray buf;

    const int entrySize = confUtil.writeZoneFlag(zoneId, enabled, buf);

    if (entrySize == 0) {
        showErrorMessage(confUtil.errorMessage());
        return false;
    }

    auto driverManager = IoC<DriverManager>();
    if (!driverManager->writeZones(buf, entrySize, true)) {
        showErrorMessage(driverManager->errorMessage());
        return false;
    }

    return true;
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

    // Load App Groups
    if (!loadAppGroups(sqliteDb(), conf))
        return false;

    return true;
}

bool ConfManager::saveToDb(const FirewallConf &conf)
{
    sqliteDb()->beginTransaction();

    const bool ok = saveAddressGroups(sqliteDb(), conf) // Save Address Groups
            && saveAppGroups(sqliteDb(), conf) // Save App Groups
            && removeAppGroupsInDb(sqliteDb(), conf); // Remove App Groups

    return checkResult(ok, /*commit=*/true);
}

void ConfManager::loadExtFlags(IniOptions &ini)
{
    ini.cacheExplorerIntegrated(StartupUtil::isExplorerIntegrated());
}

void ConfManager::saveExtFlags(const IniOptions &ini)
{
    // Windows Explorer integration
    if (ini.explorerIntegratedSet()) {
        StartupUtil::setExplorerIntegrated(ini.explorerIntegrated());
    }

    // Check password on Uninstall
    if (ini.checkPasswordOnUninstallSet() || ini.hasPasswordSet()) {
        StartupUtil::setRegistryPasswordHash(
                ini.checkPasswordOnUninstall() ? IoC<FortSettings>()->passwordHash() : QString());
    }
}

void ConfManager::saveTasksByIni(const IniOptions &ini)
{
    // Task Info List
    if (ini.taskInfoListSet()) {
        IoC<TaskManager>()->saveVariant(ini.taskInfoList());
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
    taskInfo->setIntervalHours(stmt.columnInt(2));
    taskInfo->setLastRun(stmt.columnDateTime(3));
    taskInfo->setLastSuccess(stmt.columnDateTime(4));
    taskInfo->setData(stmt.columnBlob(5));

    return true;
}

bool ConfManager::saveTask(TaskInfo *taskInfo)
{
    const bool rowExists = (taskInfo->id() != 0);

    const auto vars = QVariantList()
            << (rowExists ? taskInfo->id() : QVariant()) << taskInfo->name() << taskInfo->enabled()
            << taskInfo->intervalHours() << taskInfo->lastRun() << taskInfo->lastSuccess()
            << taskInfo->data();

    const char *sql = rowExists ? sqlUpdateTask : sqlInsertTask;

    bool ok = true;
    sqliteDb()->executeEx(sql, vars, 0, &ok);
    if (!ok)
        return false;

    if (!rowExists) {
        taskInfo->setId(sqliteDb()->lastInsertRowid());
    }
    return true;
}

bool ConfManager::checkResult(bool ok, bool commit)
{
    const auto errorMessage = ok ? QString() : sqliteDb()->errorMessage();

    if (commit) {
        sqliteDb()->endTransaction(ok);
    }

    if (!ok) {
        showErrorMessage(errorMessage);
    }

    return ok;
}
