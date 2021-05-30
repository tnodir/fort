#include "confmanager.h"

#include <QLoggingCategory>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include "../appinfo/appinfocache.h"
#include "../driver/drivercommon.h"
#include "../driver/drivermanager.h"
#include "../fortmanager.h"
#include "../fortsettings.h"
#include "../log/logentryblocked.h"
#include "../task/taskinfo.h"
#include "../task/taskmanager.h"
#include "../user/iniuser.h"
#include "../util/conf/confutil.h"
#include "../util/dateutil.h"
#include "../util/fileutil.h"
#include "../util/osutil.h"
#include "../util/startuputil.h"
#include "addressgroup.h"
#include "appgroup.h"
#include "firewallconf.h"

Q_DECLARE_LOGGING_CATEGORY(CLOG_CONF_MANAGER)
Q_LOGGING_CATEGORY(CLOG_CONF_MANAGER, "conf")

#define logWarning()  qCWarning(CLOG_CONF_MANAGER, )
#define logCritical() qCCritical(CLOG_CONF_MANAGER, )

#define DATABASE_USER_VERSION 10

namespace {

const char *const sqlSelectAddressGroups = "SELECT addr_group_id, include_all, exclude_all,"
                                           "    include_zones, exclude_zones,"
                                           "    include_text, exclude_text"
                                           "  FROM address_group"
                                           "  ORDER BY order_index;";

const char *const sqlSelectAppGroups = "SELECT app_group_id, enabled, log_conn,"
                                       "    fragment_packet, period_enabled,"
                                       "    limit_in_enabled, limit_out_enabled,"
                                       "    speed_limit_in, speed_limit_out,"
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

const char *const sqlInsertAppGroup =
        "INSERT INTO app_group(app_group_id, order_index, enabled, log_conn,"
        "    fragment_packet, period_enabled,"
        "    limit_in_enabled, limit_out_enabled,"
        "    speed_limit_in, speed_limit_out,"
        "    name, block_text, allow_text,"
        "    period_from, period_to)"
        "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15);";

const char *const sqlUpdateAppGroup = "UPDATE app_group"
                                      "  SET order_index = ?2, enabled = ?3, log_conn = ?4,"
                                      "    fragment_packet = ?5, period_enabled = ?6,"
                                      "    limit_in_enabled = ?7, limit_out_enabled = ?8,"
                                      "    speed_limit_in = ?9, speed_limit_out = ?10,"
                                      "    name = ?11, block_text = ?12, allow_text = ?13,"
                                      "    period_from = ?14, period_to = ?15"
                                      "  WHERE app_group_id = ?1;";

const char *const sqlDeleteAppGroup = "DELETE FROM app_group"
                                      "  WHERE app_group_id = ?1;";

const char *const sqlInsertService = "INSERT INTO service(name, app_group_id) VALUES(?1, ?2);";

const char *const sqlDeleteServices = "DELETE FROM service;";

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

const char *const sqlSelectApps = "SELECT"
                                  "    g.order_index as group_index,"
                                  "    t.path,"
                                  "    t.use_group_perm,"
                                  "    t.blocked,"
                                  "    (alert.app_id IS NOT NULL) as alerted"
                                  "  FROM app t"
                                  "    JOIN app_group g ON g.app_group_id = t.app_group_id"
                                  "    LEFT JOIN app_alert alert ON alert.app_id = t.app_id;";

const char *const sqlSelectEndAppsCount = "SELECT COUNT(*) FROM app"
                                          "  WHERE end_time IS NOT NULL AND end_time != 0"
                                          "    AND blocked = 0;";

const char *const sqlSelectEndedApps = "SELECT t.app_id, t.app_group_id,"
                                       "    g.order_index as group_index,"
                                       "    t.path, t.name, t.use_group_perm"
                                       "  FROM app t"
                                       "    JOIN app_group g ON g.app_group_id = t.app_group_id"
                                       "  WHERE end_time <= ?1 AND blocked = 0;";

const char *const sqlSelectAppIdByPath = "SELECT app_id FROM app WHERE path = ?1;";

const char *const sqlUpsertApp =
        "INSERT INTO app(app_group_id, path, name, use_group_perm, blocked,"
        "    creat_time, end_time)"
        "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7)"
        "  ON CONFLICT(path) DO UPDATE"
        "  SET app_group_id = ?1, name = ?3, use_group_perm = ?4, blocked = ?5,"
        "    creat_time = ?6, end_time = ?7"
        "  RETURNING app_id;";

const char *const sqlInsertAppAlert = "INSERT INTO app_alert(app_id) VALUES(?1);";

const char *const sqlDeleteApp = "DELETE FROM app WHERE app_id = ?1;";

const char *const sqlDeleteAppAlert = "DELETE FROM app_alert WHERE app_id = ?1;";

const char *const sqlUpdateApp = "UPDATE app"
                                 "  SET app_group_id = ?2, name = ?3, use_group_perm = ?4,"
                                 "    blocked = ?5, end_time = ?6"
                                 "  WHERE app_id = ?1;";

const char *const sqlUpdateAppName = "UPDATE app SET name = ?2 WHERE app_id = ?1;";

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

    // COMPAT: Zones
    if (version == 6) {
        db->execute("UPDATE task SET name = 'ZoneDownloader' WHERE name = 'Tasix';");
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
        appGroup->setLogConn(stmt.columnBool(2));
        appGroup->setFragmentPacket(stmt.columnBool(3));
        appGroup->setPeriodEnabled(stmt.columnBool(4));
        appGroup->setLimitInEnabled(stmt.columnBool(5));
        appGroup->setLimitOutEnabled(stmt.columnBool(6));
        appGroup->setSpeedLimitIn(quint32(stmt.columnInt(7)));
        appGroup->setSpeedLimitOut(quint32(stmt.columnInt(8)));
        appGroup->setName(stmt.columnText(9));
        appGroup->setBlockText(stmt.columnText(10));
        appGroup->setAllowText(stmt.columnText(11));
        appGroup->setPeriodFrom(stmt.columnText(12));
        appGroup->setPeriodTo(stmt.columnText(13));
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
            << appGroup->logConn() << appGroup->fragmentPacket() << appGroup->periodEnabled()
            << appGroup->limitInEnabled() << appGroup->limitOutEnabled() << appGroup->speedLimitIn()
            << appGroup->speedLimitOut() << appGroup->name() << appGroup->blockText()
            << appGroup->allowText() << appGroup->periodFrom() << appGroup->periodTo();

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

        db->executeEx(
                sqlUpdateAppResetGroup, QVariantList() << appGroupId << defaultAppGroupId, 0, &ok);
        if (!ok)
            return false;

        db->executeEx(sqlDeleteAppGroup, QVariantList() << appGroupId, 0, &ok);
        if (!ok)
            return false;
    }

    conf.clearRemovedAppGroupIdList();

    return true;
}

bool saveServices(SqliteDb *db, const FirewallConf &conf)
{
#if 0
    bool ok;

    // Delete services
    db->executeEx(sqlDeleteServices, {}, 0, &ok);
    if (!ok)
        return false;

    // Add services
    SqliteStmt stmt;
    if (!db->prepare(stmt, sqlInsertService))
        return false;

    for (const auto &v : conf.servicesMap()) {
    }

#endif
    return true;
}

}

ConfManager::ConfManager(
        const QString &filePath, FortManager *fortManager, QObject *parent, quint32 openFlags) :
    QObject(parent),
    m_fortManager(fortManager),
    m_sqliteDb(new SqliteDb(filePath, openFlags)),
    m_conf(createConf())
{
    connect(&m_appAlertedTimer, &QTimer::timeout, this, &ConfManager::appAlerted);
    connect(&m_appChangedTimer, &QTimer::timeout, this, &ConfManager::appChanged);
    connect(&m_appUpdatedTimer, &QTimer::timeout, this, &ConfManager::appUpdated);

    m_appEndTimer.setInterval(5 * 60 * 1000); // 5 minutes
    connect(&m_appEndTimer, &QTimer::timeout, this, &ConfManager::checkAppEndTimes);
}

ConfManager::~ConfManager()
{
    delete m_sqliteDb;
}

DriverManager *ConfManager::driverManager() const
{
    return fortManager()->driverManager();
}

EnvManager *ConfManager::envManager() const
{
    return fortManager()->envManager();
}

FortSettings *ConfManager::settings() const
{
    return fortManager()->settings();
}

TaskManager *ConfManager::taskManager() const
{
    return fortManager()->taskManager();
}

AppInfoCache *ConfManager::appInfoCache() const
{
    return fortManager()->appInfoCache();
}

IniUser *ConfManager::iniUser() const
{
    return fortManager()->iniUser();
}

void ConfManager::showErrorMessage(const QString &errorMessage)
{
    logWarning() << "Error:" << errorMessage;

    if (!settings()->isService()) {
        fortManager()->showErrorBox(errorMessage, tr("Configuration Error"));
    }
}

bool ConfManager::initialize()
{
    if (!sqliteDb()->open()) {
        logCritical() << "File open error:" << sqliteDb()->filePath() << sqliteDb()->errorMessage();
        return false;
    }

    if (!sqliteDb()->migrate(
                ":/conf/migrations", nullptr, DATABASE_USER_VERSION, true, true, &migrateFunc)) {
        logCritical() << "Migration error" << sqliteDb()->filePath();
        return false;
    }

    setupAppEndTimer();

    return true;
}

void ConfManager::setupAppEndTimer()
{
    m_appEndTimer.start();
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
    FirewallConf *conf = new FirewallConf(settings(), this);
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

    settings()->readConfIni(conf);

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

    settings()->writeConfIni(conf);

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

    conf()->resetEdited();
}

bool ConfManager::save(FirewallConf *newConf)
{
    if (!saveConf(*newConf))
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
        emit iniUserChanged(true);
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

    const QString appName = appInfoCache()->appName(appPath);
    constexpr int groupIndex = 0; // "Main" app. group

    addApp(appPath, appName, QDateTime(), groupIndex, false, logEntry.blocked(), true);
}

qint64 ConfManager::appIdByPath(const QString &appPath)
{
    return sqliteDb()->executeEx(sqlSelectAppIdByPath, { appPath }).toLongLong();
}

bool ConfManager::addApp(const QString &appPath, const QString &appName, const QDateTime &endTime,
        int groupIndex, bool useGroupPerm, bool blocked, bool alerted)
{
    const AppGroup *appGroup = conf()->appGroupAt(groupIndex);
    if (appGroup->isNull())
        return false;

    if (!alerted && !updateDriverUpdateApp(appPath, groupIndex, useGroupPerm, blocked))
        return false;

    bool ok = false;

    sqliteDb()->beginTransaction();

    const auto vars = QVariantList()
            << appGroup->id() << appPath << appName << useGroupPerm << blocked
            << QDateTime::currentDateTime() << (!endTime.isNull() ? endTime : QVariant());

    const auto appIdVar = sqliteDb()->executeEx(sqlUpsertApp, vars, 1, &ok);

    if (ok) {
        // Alert
        const qint64 appId = appIdVar.toLongLong();
        sqliteDb()->executeEx(alerted ? sqlInsertAppAlert : sqlDeleteAppAlert, { appId });
    }

    checkResult(ok, true);

    if (ok) {
        if (!endTime.isNull()) {
            m_appEndTimer.start();
        }

        if (alerted) {
            emitAppAlerted();
        }
        emitAppChanged();
    }

    return ok;
}

bool ConfManager::deleteApp(qint64 appId, const QString &appPath)
{
    if (!updateDriverDeleteApp(appPath))
        return false;

    bool ok = false;

    sqliteDb()->beginTransaction();

    const auto vars = QVariantList() << appId;

    sqliteDb()->executeEx(sqlDeleteApp, vars, 0, &ok);
    if (ok) {
        sqliteDb()->executeEx(sqlDeleteAppAlert, vars, 0, &ok);
    }

    checkResult(ok, true);

    if (ok) {
        emitAppChanged();
    }

    return ok;
}

bool ConfManager::updateApp(qint64 appId, const QString &appPath, const QString &appName,
        const QDateTime &endTime, qint64 groupId, int groupIndex, bool useGroupPerm, bool blocked)
{
    if (!updateDriverUpdateApp(appPath, groupIndex, useGroupPerm, blocked))
        return false;

    bool ok = false;

    sqliteDb()->beginTransaction();

    const auto vars = QVariantList() << appId << groupId << appName << useGroupPerm << blocked
                                     << (!endTime.isNull() ? endTime : QVariant());

    sqliteDb()->executeEx(sqlUpdateApp, vars, 0, &ok);
    if (ok) {
        sqliteDb()->executeEx(sqlDeleteAppAlert, { appId }, 0, &ok);
    }

    checkResult(ok, true);

    if (ok) {
        if (!endTime.isNull()) {
            m_appEndTimer.start();
        }

        emit appUpdated();
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
        emit appUpdated();
    }

    return ok;
}

bool ConfManager::walkApps(const std::function<walkAppsCallback> &func)
{
    SqliteStmt stmt;
    if (!sqliteDb()->prepare(stmt, sqlSelectApps))
        return false;

    while (stmt.step() == SqliteStmt::StepRow) {
        const int groupIndex = stmt.columnInt(0);
        const QString appPath = stmt.columnText(1);
        const bool useGroupPerm = stmt.columnBool(2);
        const bool blocked = stmt.columnBool(3);
        const bool alerted = stmt.columnBool(4);

        if (!func(groupIndex, useGroupPerm, blocked, alerted, appPath))
            return false;
    }

    return true;
}

int ConfManager::appEndsCount()
{
    return sqliteDb()->executeEx(sqlSelectEndAppsCount).toInt();
}

void ConfManager::updateAppEndTimes()
{
    SqliteStmt stmt;
    if (!stmt.prepare(sqliteDb()->db(), sqlSelectEndedApps))
        return;

    stmt.bindDateTime(1, QDateTime::currentDateTime());

    bool isAppEndTimesUpdated = false;

    while (stmt.step() == SqliteStmt::StepRow) {
        const qint64 appId = stmt.columnInt64(0);
        const qint64 groupId = stmt.columnInt64(1);
        const int groupIndex = stmt.columnInt(2);
        const QString appPath = stmt.columnText(3);
        const QString appName = stmt.columnText(4);
        const bool useGroupPerm = stmt.columnBool(5);

        if (updateApp(appId, appPath, appName, QDateTime(), groupId, groupIndex, useGroupPerm,
                    true)) {
            isAppEndTimesUpdated = true;
        }
    }

    if (isAppEndTimesUpdated) {
        emitAppUpdated();
    }
}

void ConfManager::checkAppEndTimes()
{
    if (appEndsCount() == 0) {
        m_appEndTimer.stop();
    } else {
        updateAppEndTimes();
    }
}

bool ConfManager::addZone(const QString &zoneName, const QString &sourceCode, const QString &url,
        const QString &formData, bool enabled, bool customUrl, int &zoneId)
{
    bool ok = false;

    zoneId = getFreeZoneId();

    const auto vars = QVariantList()
            << zoneId << zoneName << enabled << customUrl << sourceCode << url << formData;

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

bool ConfManager::updateZone(int zoneId, const QString &zoneName, const QString &sourceCode,
        const QString &url, const QString &formData, bool enabled, bool customUrl)
{
    if (!updateDriverZoneFlag(zoneId, enabled))
        return false;

    bool ok = false;

    const auto vars = QVariantList()
            << zoneId << zoneName << enabled << customUrl << sourceCode << url << formData;

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

bool ConfManager::updateZoneResult(int zoneId, int addressCount, const QString &textChecksum,
        const QString &binChecksum, const QDateTime &sourceModTime, const QDateTime &lastRun,
        const QDateTime &lastSuccess)
{
    bool ok = false;

    const auto vars = QVariantList() << zoneId << addressCount << textChecksum << binChecksum
                                     << sourceModTime << lastRun << lastSuccess;

    sqliteDb()->executeEx(sqlUpdateZoneResult, vars, 0, &ok);

    checkResult(ok);

    if (ok) {
        emit zoneUpdated();
    }

    return ok;
}

bool ConfManager::checkPassword(const QString &password)
{
    return settings()->checkPassword(password);
}

bool ConfManager::validateDriver()
{
    ConfUtil confUtil;
    QByteArray buf;

    const int verSize = confUtil.writeVersion(buf);

    return driverManager()->validate(buf, verSize);
}

bool ConfManager::updateDriverConf(bool onlyFlags)
{
    ConfUtil confUtil;
    QByteArray buf;

    const int confSize = onlyFlags ? confUtil.writeFlags(*conf(), buf)
                                   : confUtil.write(*conf(), this, *envManager(), buf);
    if (confSize == 0) {
        showErrorMessage(confUtil.errorMessage());
        return false;
    }

    if (!driverManager()->writeConf(buf, confSize, onlyFlags)) {
        showErrorMessage(driverManager()->errorMessage());
        return false;
    }

    return true;
}

bool ConfManager::updateDriverDeleteApp(const QString &appPath)
{
    return updateDriverUpdateApp(appPath, 0, false, false, true);
}

bool ConfManager::updateDriverUpdateApp(
        const QString &appPath, int groupIndex, bool useGroupPerm, bool blocked, bool remove)
{
    ConfUtil confUtil;
    QByteArray buf;

    const int entrySize =
            confUtil.writeAppEntry(groupIndex, useGroupPerm, blocked, false, false, appPath, buf);

    if (entrySize == 0) {
        showErrorMessage(confUtil.errorMessage());
        return false;
    }

    if (!driverManager()->writeApp(buf, entrySize, remove)) {
        showErrorMessage(driverManager()->errorMessage());
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

    if (!driverManager()->writeZones(buf, entrySize)) {
        showErrorMessage(driverManager()->errorMessage());
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

    if (!driverManager()->writeZones(buf, entrySize, true)) {
        showErrorMessage(driverManager()->errorMessage());
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
            && removeAppGroupsInDb(sqliteDb(), conf) // Remove App Groups
            && saveServices(sqliteDb(), conf); // Save Services

    return checkResult(ok, true);
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
