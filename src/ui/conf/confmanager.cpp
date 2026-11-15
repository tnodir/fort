#include "confmanager.h"

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
#include <util/fileutil.h>

#include "addressgroup.h"
#include "confappmanager.h"
#include "confgroupmanager.h"
#include "confmigration.h"
#include "confrulemanager.h"
#include "conftimeperiodmanager.h"
#include "filtersimconn.h"

using namespace Fort;

namespace {

const QLoggingCategory LC("conf");

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

bool ConfManager::writeConfBuffer(ConfBuffer &confBuf, const std::function<bool()> &write)
{
    confBuf.setHostRefs(&m_hostRefs);

    const bool ok = write();

    checkHostRefsChanged();

    return ok;
}

void ConfManager::checkHostRefsChanged()
{
    if (!m_hostRefs.isChanged())
        return;

    m_hostRefs.resetChanged();

    emit hostRefsChanged();
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
    const int autoLearnMsec = conf().allowAllNew() ? ini().autoLearnSeconds() * 1000 : 0;

    const bool isTimerActive =
            (m_autoLearnTimer.isActive() && m_autoLearnTimer.interval() == autoLearnMsec);
    if (isTimerActive)
        return;

    m_autoLearnTimer.stop();

    if (autoLearnMsec > 0) {
        m_autoLearnTimer.start(autoLearnMsec);
    }
}

void ConfManager::clearInitialAutoLearn()
{
    ini().saveInitialAutoLearnSeconds(0);
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
    conf().setFilterMode(FirewallConf::ModeAskToConnect);

    saveFlags();
}

void ConfManager::updateAutoLearnOff()
{
    const bool isAutoLearn = conf().allowAllNew();

    if (m_savedAutoLearn && !isAutoLearn) {
        conf().setAutoLearnOff();
        clearInitialAutoLearn();
    }

    m_savedAutoLearn = isAutoLearn;
}

bool ConfManager::setupDb()
{
    if (!sqliteDb()->open()) {
        qCCritical(LC) << "File open error:" << sqliteDb()->filePath()
                       << sqliteDb()->errorMessage();
        return false;
    }

    SqliteDb::MigrateOptions opt = ConfMigration::migrateOptions();

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

void ConfManager::setupDefaultPresetRules(bool isNewConf)
{
    confRuleManager()->syncPresetRules(
            isNewConf ? PresetRulesManager::SyncRestore : PresetRulesManager::SyncUpdate);
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

        setupDefaultPresetRules(isNewConf);
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

    m_savedAutoLearn = false; // a loaded mode doesn't leave Auto-Learn

    applySavedConf(conf());
}

void ConfManager::reload()
{
    // The edited flags are reset after the first load()
    conf().resetEdited(FirewallConf::AllEdited);

    load();
}

bool ConfManager::save(FirewallConf &newConf, IniOptions &ini)
{
    setErrorMessage({});

    if (!newConf.anyEdited())
        return true;

    if (&conf() != &newConf) {
        newConf.takeUneditedFlags(conf());
    }

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

    updateAutoLearnOff();

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
    conf().setFlagsEdited(FirewallConf::AllFlagGroups);

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
    SqliteDb::MigrateOptions opt = ConfMigration::migrateOptions();

    opt.backupFilePath = path + FileUtil::fileName(sqliteDb()->filePath());

    if (!sqliteDb()->import(opt))
        return false;

    emit imported();

    reload();

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

    const FORT_CONF_APP_FIND_RESULT app = DriverCommon::confAppFind(drvConf, simConn.appPath);

    simConn.result = DriverCommon::confConnFilter(cf, &simConn.conn, app);
    simConn.conn.app.rule = nullptr; // points into the local conf buffer

    return true;
}

bool ConfManager::validateConf(const FirewallConf &conf)
{
    if (!conf.optEdited())
        return true;

    ConfBuffer confBuf;

    if (!confBuf.writeConf(conf, confAppManager(), envManager())) {
        setErrorMessage(confBuf.errorMessage());
        qCCritical(LC) << "Conf save error:" << errorMessage();
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
