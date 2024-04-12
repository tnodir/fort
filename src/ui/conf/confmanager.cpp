#include "confmanager.h"

#include <QHash>
#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/dbvar.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <appinfo/appinfocache.h>
#include <appinfo/appinfoutil.h>
#include <driver/drivermanager.h>
#include <fortsettings.h>
#include <log/logmanager.h>
#include <manager/envmanager.h>
#include <manager/serviceinfomanager.h>
#include <manager/servicemanager.h>
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

constexpr int DATABASE_USER_VERSION = 41;

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

const char *const sqlSelectTaskByName = "SELECT task_id, enabled, run_on_startup, interval_hours,"
                                        "    last_run, last_success, data"
                                        "  FROM task"
                                        "  WHERE name = ?1;";

const char *const sqlInsertTask = "INSERT INTO task(task_id, name, enabled, run_on_startup,"
                                  "    interval_hours, last_run, last_success, data)"
                                  "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8);";

const char *const sqlUpdateTask = "UPDATE task"
                                  "  SET name = ?2, enabled = ?3, run_on_startup = ?4,"
                                  "    interval_hours = ?5, last_run = ?6, last_success = ?7,"
                                  "    data = ?8"
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

bool saveAddressGroup(SqliteDb *db, AddressGroup *addrGroup, int orderIndex)
{
    const bool rowExists = (addrGroup->id() != 0);
    if (!addrGroup->edited() && rowExists)
        return true;

    const QVariantList vars = {
        DbVar::nullable(addrGroup->id(), !rowExists),
        orderIndex,
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
    if (!DbQuery(db).sql(sqlSelectAppGroups).prepare(stmt))
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

    const QVariantList vars = {
        DbVar::nullable(appGroup->id(), !rowExists),
        orderIndex,
        appGroup->enabled(),
        appGroup->applyChild(),
        appGroup->lanOnly(),
        appGroup->logBlocked(),
        appGroup->logConn(),
        appGroup->periodEnabled(),
        appGroup->limitInEnabled(),
        appGroup->limitOutEnabled(),
        appGroup->speedLimitIn(),
        appGroup->speedLimitOut(),
        appGroup->limitPacketLoss(),
        appGroup->limitLatency(),
        appGroup->limitBufferSizeIn(),
        appGroup->limitBufferSizeOut(),
        appGroup->name(),
        appGroup->killText(),
        appGroup->blockText(),
        appGroup->allowText(),
        appGroup->periodFrom(),
        appGroup->periodTo(),
    };

    const char *sql = rowExists ? sqlUpdateAppGroup : sqlInsertAppGroup;

    if (!DbQuery(db).sql(sql).vars(vars).executeOk())
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
        DbQuery(db).sql(sqlUpdateAppResetGroup).vars({ appGroupId, defaultAppGroupId }).executeOk();

        if (!DbQuery(db).sql(sqlDeleteAppGroup).vars({ appGroupId }).executeOk())
            return false;
    }

    conf.clearRemovedAppGroupIdList();

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

    if (!FileUtil::replaceFile(srcFilePath, filePath)) {
        qCWarning(LC) << "Import file error from:" << srcFilePath << "to:" << filePath;
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
    setupDb();
}

void ConfManager::initConfToEdit()
{
    if (confToEdit())
        return;

    auto newConf = createConf();
    newConf->copy(*conf());

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

bool ConfManager::setupDb()
{
    if (!sqliteDb()->open()) {
        qCCritical(LC) << "File open error:" << sqliteDb()->filePath()
                       << sqliteDb()->errorMessage();
        return false;
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
                        .columns = { "path", "name", "notes" }
                },
                {
                        .contentTable = "rule",
                        .contentRowid = "rule_id",
                        .columns = { "name", "notes" }
                },
                },
        };

    if (!sqliteDb()->migrate(opt)) {
        qCCritical(LC) << "Migration error" << sqliteDb()->filePath();
        return false;
    }

    return true;
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

        if (!loadFromDb(conf, isNewConf))
            return false;

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
    iniUser().saveAndClear();

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

    commitTransaction(ok);

    return ok;
}

bool ConfManager::exportBackup(const QString &path)
{
    // Export User Ini
    {
        if (!exportFile(iniUser().settings()->filePath(), path))
            return false;
    }

    return exportMasterBackup(path);
}

bool ConfManager::exportMasterBackup(const QString &path)
{
    // Export Ini
    {
        if (!exportFile(conf()->ini().settings()->filePath(), path))
            return false;
    }

    // Export Db
    {
        const QString fileName = FileUtil::fileName(sqliteDb()->filePath());
        const QString destFilePath = path + fileName;

        FileUtil::removeFile(destFilePath);
        if (!sqliteDb()->vacuumInto(destFilePath)) {
            qCWarning(LC) << "Export Db error:" << sqliteDb()->errorMessage() << "to:" << path;
            return false;
        }
    }

    return true;
}

bool ConfManager::importBackup(const QString &path)
{
    // Import User Ini
    {
        if (!importFile(iniUser().settings()->filePath(), path))
            return false;

        iniUser().settings()->clearCache();
    }

    // Import Db: Close DB from UI side
    {
        sqliteDb()->close();
    }

    return importMasterBackup(path);
}

bool ConfManager::importMasterBackup(const QString &path)
{
    bool ok;

    // Import Ini
    {
        Settings *settings = conf()->ini().settings();

        ok = importFile(settings->filePath(), path);

        settings->clearCache();
    }

    // Import Db
    if (ok) {
        sqliteDb()->close();

        ok = importFile(sqliteDb()->filePath(), path);
    }

    if (!ok) {
        qCWarning(LC) << "Import error from:" << path;
    }

    if (IoC<FortSettings>()->isService()) {
        IoC<ServiceManager>()->restart();
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

    const int confSize = confUtil.write(newConf, IoC<ConfAppManager>(), *IoC<EnvManager>());
    if (confSize == 0) {
        qCCritical(LC) << "Conf save error:" << confUtil.errorMessage();
        return false;
    }

    return true;
}

bool ConfManager::validateDriver()
{
    ConfUtil confUtil;

    confUtil.writeVersion();

    return IoC<DriverManager>()->validate(confUtil.buffer());
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

    confUtil.writeServices(services, runningServicesCount);

    IoC<DriverManager>()->writeServices(confUtil.buffer());
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

    bool ok = saveAddressGroups(sqliteDb(), conf) // Save Address Groups
            && saveAppGroups(sqliteDb(), conf) // Save App Groups
            && removeAppGroupsInDb(sqliteDb(), conf); // Remove App Groups

    commitTransaction(ok);

    return ok;
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
    taskInfo->setRunOnStatup(stmt.columnBool(2));
    taskInfo->setIntervalHours(stmt.columnInt(3));
    taskInfo->setLastRun(stmt.columnDateTime(4));
    taskInfo->setLastSuccess(stmt.columnDateTime(5));
    taskInfo->setData(stmt.columnBlob(6));

    return true;
}

bool ConfManager::saveTask(TaskInfo *taskInfo)
{
    const bool rowExists = (taskInfo->id() != 0);

    const QVariantList vars = {
        DbVar::nullable(taskInfo->id(), !rowExists),
        taskInfo->name(),
        taskInfo->enabled(),
        taskInfo->runOnStatup(),
        taskInfo->intervalHours(),
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

bool ConfManager::beginTransaction()
{
    return sqliteDb()->beginWriteTransaction();
}

void ConfManager::commitTransaction(bool &ok)
{
    ok = sqliteDb()->endTransaction(ok);
}
