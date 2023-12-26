#include "confmanager.h"

#include <QHash>
#include <QLoggingCategory>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <appinfo/appinfocache.h>
#include <appinfo/appinfoutil.h>
#include <conf/zone.h>
#include <driver/drivermanager.h>
#include <fortsettings.h>
#include <log/logmanager.h>
#include <manager/envmanager.h>
#include <manager/serviceinfomanager.h>
#include <manager/windowmanager.h>
#include <task/taskinfo.h>
#include <task/taskmanager.h>
#include <user/iniuser.h>
#include <user/usersettings.h>
#include <util/conf/confutil.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>
#include <util/startuputil.h>

#include "addressgroup.h"
#include "appgroup.h"
#include "confappmanager.h"
#include "firewallconf.h"

namespace {

const QLoggingCategory LC("conf");

constexpr int DATABASE_USER_VERSION = 27;

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
                                       "    name, kill_text, block_text, allow_text,"
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
                                      "    name, kill_text, block_text, allow_text,"
                                      "    period_from, period_to)"
                                      "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12,"
                                      "    ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20, ?21, ?22);";

const char *const sqlUpdateAppGroup = "UPDATE app_group"
                                      "  SET order_index = ?2, enabled = ?3,"
                                      "    apply_child = ?4, lan_only = ?5,"
                                      "    log_blocked = ?6, log_conn = ?7, period_enabled = ?8,"
                                      "    limit_in_enabled = ?9, limit_out_enabled = ?10,"
                                      "    speed_limit_in = ?11, speed_limit_out = ?12,"
                                      "    limit_packet_loss = ?13, limit_latency = ?14,"
                                      "    limit_bufsize_in = ?15, limit_bufsize_out = ?16,"
                                      "    name = ?17, kill_text = ?18, block_text = ?19,"
                                      "    allow_text = ?20, period_from = ?21, period_to = ?22"
                                      "  WHERE app_group_id = ?1;";

const char *const sqlDeleteAppGroup = "DELETE FROM app_group"
                                      "  WHERE app_group_id = ?1;";

const char *const sqlUpdateAppResetGroup = "UPDATE app"
                                           "  SET app_group_id = ?2"
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

const char *const sqlInsertZone = "INSERT INTO zone(zone_id, name, enabled, custom_url,"
                                  "    source_code, url, form_data, text_inline)"
                                  "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8);";

const char *const sqlSelectZoneIds = "SELECT zone_id FROM zone ORDER BY zone_id;";

const char *const sqlDeleteZone = "DELETE FROM zone WHERE zone_id = ?1;";

const char *const sqlDeleteAddressGroupZone = "UPDATE address_group"
                                              "  SET include_zones = include_zones & ?1,"
                                              "    exclude_zones = exclude_zones & ?1;";

const char *const sqlDeleteAppZone = "UPDATE app"
                                     "  SET accept_zones = accept_zones & ?1,"
                                     "    reject_zones = reject_zones & ?1;";

const char *const sqlUpdateZone = "UPDATE zone"
                                  "  SET name = ?2, enabled = ?3, custom_url = ?4,"
                                  "    source_code = ?5, url = ?6,"
                                  "    form_data = ?7, text_inline = ?8"
                                  "  WHERE zone_id = ?1;";

const char *const sqlUpdateZoneName = "UPDATE zone SET name = ?2 WHERE zone_id = ?1;";

const char *const sqlUpdateZoneEnabled = "UPDATE zone SET enabled = ?2 WHERE zone_id = ?1;";

const char *const sqlUpdateZoneResult =
        "UPDATE zone"
        "  SET address_count = ?2, text_checksum = ?3, bin_checksum = ?4,"
        "    source_modtime = ?5, last_run = ?6, last_success = ?7"
        "  WHERE zone_id = ?1;";

using AppsMap = QHash<qint64, QString>;
using AppIdsArray = QVector<qint64>;

bool fillAppPathsMap(SqliteDb *db, AppsMap &appsMap)
{
    const char *const sql = "SELECT app_id, origin_path, path FROM app;";

    SqliteStmt stmt;
    if (!db->prepare(stmt, sql))
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
    if (!db->prepare(stmt, sqlUpdateAppPaths))
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
        db->executeEx(sqlDeleteApp, { appId });
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

    return true;
}

void showErrorMessage(const QString &errorMessage)
{
    IoC<WindowManager>()->showErrorBox(errorMessage, ConfManager::tr("Configuration Error"));
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

    if (!db->executeExOk(sql, vars))
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
        appGroup->setKillText(stmt.columnText(16));
        appGroup->setBlockText(stmt.columnText(17));
        appGroup->setAllowText(stmt.columnText(18));
        appGroup->setPeriodFrom(stmt.columnText(19));
        appGroup->setPeriodTo(stmt.columnText(20));
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
            << appGroup->killText() << appGroup->blockText() << appGroup->allowText()
            << appGroup->periodFrom() << appGroup->periodTo();

    const char *sql = rowExists ? sqlUpdateAppGroup : sqlInsertAppGroup;

    if (!db->executeExOk(sql, vars))
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
        db->executeEx(sqlUpdateAppResetGroup, { appGroupId, defaultAppGroupId }, 0);

        if (!db->executeExOk(sqlDeleteAppGroup, { appGroupId }))
            return false;
    }

    conf.clearRemovedAppGroupIdList();

    return true;
}

bool driverWriteZones(ConfUtil &confUtil, QByteArray &buf, int entrySize, bool onlyFlags = false)
{
    if (entrySize == 0) {
        showErrorMessage(confUtil.errorMessage());
        return false;
    }

    auto driverManager = IoC<DriverManager>();
    if (!driverManager->writeZones(buf, entrySize, onlyFlags)) {
        showErrorMessage(driverManager->errorMessage());
        return false;
    }

    return true;
}

}

ConfManager::ConfManager(const QString &filePath, QObject *parent, quint32 openFlags) :
    QObject(parent), m_sqliteDb(new SqliteDb(filePath, openFlags)), m_conf(createConf())
{
}

IniUser &ConfManager::iniUser() const
{
    return IoC<UserSettings>()->iniUser();
}

void ConfManager::setUp()
{
    if (!sqliteDb()->open()) {
        qCCritical(LC) << "File open error:" << sqliteDb()->filePath()
                       << sqliteDb()->errorMessage();
        return;
    }

    SqliteDb::MigrateOptions opt = {
        .sqlDir = ":/conf/migrations",
        .version = DATABASE_USER_VERSION,
        .recreate = true,
        .migrateFunc = &migrateFunc,
        .ftsTables = {
            {
                .contentTable = "app",
                .contentRowid = "app_id",
                .columns = { "path", "name" }
            },
        },
    };

    if (!sqliteDb()->migrate(opt)) {
        qCCritical(LC) << "Migration error" << sqliteDb()->filePath();
        return;
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

    auto newIniUser = new IniUser(iniUser().settings());

    setIniUserToEdit(newIniUser);
}

void ConfManager::setIniUserToEdit(IniUser *iniUser)
{
    if (iniUserToEdit() == iniUser)
        return;

    if (iniUserToEdit() && iniUserToEdit() != &this->iniUser()) {
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
    qCDebug(LC) << "Conf save";

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

void ConfManager::saveIniUser(bool edited, bool onlyFlags)
{
    iniUser().save();
    iniUser().clear();

    if (edited) {
        emit iniUserChanged(iniUser(), onlyFlags);
    }
}

QVariant ConfManager::toPatchVariant(bool onlyFlags) const
{
    return onlyFlags ? conf()->toVariant(/*onlyEdited=*/true) // send only flags to clients
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

    conf->fromVariant(confVar, /*onlyEdited=*/true);

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

    beginTransaction();

    for (TaskInfo *taskInfo : taskInfos) {
        if (!saveTask(taskInfo)) {
            ok = false;
            break;
        }
    }

    return commitTransaction(ok);
}

bool ConfManager::exportBackup(const QString &path)
{
    FileUtil::makePath(path);

    const QString outPath = FileUtil::pathSlash(path);

    // Export Db
    {
        const QString fileName = FileUtil::fileName(sqliteDb()->filePath());
        const QString destPath = outPath + fileName;

        FileUtil::removeFile(destPath);
        if (!sqliteDb()->vacuumInto(destPath)) {
            qCWarning(LC) << "Export Db error:" << sqliteDb()->errorMessage() << "to:" << destPath;
            return false;
        }
    }

    // Export Ini
    {
        const QString iniPath = conf()->ini().settings()->filePath();
        const QString fileName = FileUtil::fileName(iniPath);
        const QString destPath = outPath + fileName;

        FileUtil::removeFile(destPath);
        if (!FileUtil::copyFile(iniPath, outPath + fileName)) {
            qCWarning(LC) << "Copy Ini error from:" << iniPath << "to:" << destPath;
            return false;
        }
    }

    // Export User Ini
    {
        const QString iniPath = iniUser().settings()->filePath();
        const QString fileName = FileUtil::fileName(iniPath);
        const QString destPath = outPath + fileName;

        FileUtil::removeFile(destPath);
        if (!FileUtil::copyFile(iniPath, outPath + fileName)) {
            qCWarning(LC) << "Copy User Ini error from:" << iniPath << "to:" << destPath;
            return false;
        }
    }

    return true;
}

bool ConfManager::importBackup(const QString &path)
{
    return false;
}

bool ConfManager::addZone(Zone &zone)
{
    bool ok = false;

    zone.zoneId = getFreeZoneId();

    const auto vars = QVariantList()
            << zone.zoneId << zone.zoneName << zone.enabled << zone.customUrl << zone.sourceCode
            << zone.url << zone.formData << zone.textInline;

    sqliteDb()->executeEx(sqlInsertZone, vars, 0, &ok);

    checkEndTransaction(ok);

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

    beginTransaction();

    sqliteDb()->executeEx(sqlDeleteZone, { zoneId }, 0, &ok);
    if (ok) {
        const quint32 zoneUnMask = ~(quint32(1) << (zoneId - 1));

        // Delete the Zone from Address Groups
        sqliteDb()->executeEx(sqlDeleteAddressGroupZone, { qint64(zoneUnMask) }, 0, &ok);

        // Delete the Zone from Programs
        sqliteDb()->executeEx(sqlDeleteAppZone, { qint64(zoneUnMask) }, 0, &ok);
    }

    commitTransaction(ok);

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
            << zone.url << zone.formData << zone.textInline;

    sqliteDb()->executeEx(sqlUpdateZone, vars, 0, &ok);

    checkEndTransaction(ok);

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

    checkEndTransaction(ok);

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

    checkEndTransaction(ok);

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

    checkEndTransaction(ok);

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

    const int confSize = confUtil.write(newConf, IoC<ConfAppManager>(), *IoC<EnvManager>(), buf);
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

void ConfManager::updateServices()
{
    auto serviceInfoManager = IoC<ServiceInfoManager>();

    int runningServicesCount = 0;
    const QVector<ServiceInfo> services =
            serviceInfoManager->loadServiceInfoList(ServiceInfo::TypeWin32, ServiceInfo::StateAll,
                    /*displayName=*/false, &runningServicesCount);

    serviceInfoManager->monitorServices(services);

    if (runningServicesCount > 0) {
        updateDriverServices(services, runningServicesCount);
    }
}

void ConfManager::updateDriverServices(
        const QVector<ServiceInfo> &services, int runningServicesCount)
{
    ConfUtil confUtil;
    QByteArray buf;

    const int outSize = confUtil.writeServices(services, runningServicesCount, buf);

    auto driverManager = IoC<DriverManager>();
    driverManager->writeServices(buf, outSize);
}

void ConfManager::updateDriverZones(quint32 zonesMask, quint32 enabledMask, quint32 dataSize,
        const QList<QByteArray> &zonesData)
{
    ConfUtil confUtil;
    QByteArray buf;

    const int entrySize = confUtil.writeZones(zonesMask, enabledMask, dataSize, zonesData, buf);

    driverWriteZones(confUtil, buf, entrySize);
}

bool ConfManager::updateDriverZoneFlag(int zoneId, bool enabled)
{
    ConfUtil confUtil;
    QByteArray buf;

    const int entrySize = confUtil.writeZoneFlag(zoneId, enabled, buf);

    return driverWriteZones(confUtil, buf, entrySize, /*onlyFlags=*/true);
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
    beginTransaction();

    const bool ok = saveAddressGroups(sqliteDb(), conf) // Save Address Groups
            && saveAppGroups(sqliteDb(), conf) // Save App Groups
            && removeAppGroupsInDb(sqliteDb(), conf); // Remove App Groups

    return commitTransaction(ok);
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

bool ConfManager::beginTransaction()
{
    return sqliteDb()->beginTransaction();
}

bool ConfManager::commitTransaction(bool ok)
{
    ok = sqliteDb()->endTransaction(ok);

    return checkEndTransaction(ok);
}

bool ConfManager::checkEndTransaction(bool ok)
{
    if (!ok) {
        showErrorMessage(sqliteDb()->errorMessage());
    }

    return ok;
}
