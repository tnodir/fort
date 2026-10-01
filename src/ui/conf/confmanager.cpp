#include "confmanager.h"

#include <QHash>
#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/dbvar.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <driver/drivermanager.h>
#include <fortglobal.h>
#include <fortsettings.h>
#include <manager/envmanager.h>
#include <manager/serviceinfomanager.h>
#include <manager/windowmanager.h>
#include <task/taskinfo.h>
#include <task/taskmanager.h>
#include <user/usersettings.h>
#include <util/conf/confbuffer.h>
#include <util/fileutil.h>

#include "addressgroup.h"
#include "confappmanager.h"

using namespace Fort;

namespace {

const QLoggingCategory LC("conf");

inline constexpr int DATABASE_USER_VERSION = 59;

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
                                        "    max_retries, retry_seconds, interval_hours,"
                                        "    last_run, last_success, data"
                                        "  FROM task"
                                        "  WHERE name = ?1;";

const char *const sqlInsertTask = "INSERT INTO task(task_id, name, enabled,"
                                  "    run_on_startup, delay_startup,"
                                  "    max_retries, retry_seconds, interval_hours,"
                                  "    last_run, last_success, data)"
                                  "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11);";

const char *const sqlUpdateTask = "UPDATE task"
                                  "  SET name = ?2, enabled = ?3,"
                                  "    run_on_startup = ?4, delay_startup = ?5,"
                                  "    max_retries = ?6, retry_seconds = ?7, interval_hours = ?8,"
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
    int processCount = 0;
    const QVector<ServiceInfo> services = serviceInfoManager->loadServiceInfoList(
            ServiceInfo::TypeWin32OwnProcess, ServiceInfo::StateAll,
            /*displayName=*/false, &processCount);

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
    taskInfo->setIntervalHours(stmt.columnInt(6));
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
