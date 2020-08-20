#include "confmanager.h"

#include <QLoggingCategory>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include "../driver/drivermanager.h"
#include "../fortcommon.h"
#include "../fortmanager.h"
#include "../fortsettings.h"
#include "../task/taskinfo.h"
#include "../util/conf/confutil.h"
#include "../util/dateutil.h"
#include "../util/fileutil.h"
#include "../util/osutil.h"
#include "addressgroup.h"
#include "appgroup.h"
#include "firewallconf.h"

Q_DECLARE_LOGGING_CATEGORY(CLOG_CONF_MANAGER)
Q_LOGGING_CATEGORY(CLOG_CONF_MANAGER, "fort.confManager")

#define logWarning()  qCWarning(CLOG_CONF_MANAGER, )
#define logCritical() qCCritical(CLOG_CONF_MANAGER, )

#define DATABASE_USER_VERSION 7

namespace {

const char *const sqlPragmas = "PRAGMA journal_mode = WAL;"
                               "PRAGMA locking_mode = EXCLUSIVE;"
                               "PRAGMA synchronous = NORMAL;";

const char *const sqlSelectAddressGroups = "SELECT addr_group_id, include_all, exclude_all,"
                                           "    include_zones, exclude_zones,"
                                           "    include_text, exclude_text"
                                           "  FROM address_group"
                                           "  ORDER BY order_index;";

const char *const sqlSelectAppGroups = "SELECT app_group_id, enabled,"
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
        "INSERT INTO app_group(app_group_id, order_index, enabled,"
        "    fragment_packet, period_enabled,"
        "    limit_in_enabled, limit_out_enabled,"
        "    speed_limit_in, speed_limit_out,"
        "    name, block_text, allow_text,"
        "    period_from, period_to)"
        "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14);";

const char *const sqlUpdateAppGroup = "UPDATE app_group"
                                      "  SET order_index = ?2, enabled = ?3,"
                                      "    fragment_packet = ?4, period_enabled = ?5,"
                                      "    limit_in_enabled = ?6, limit_out_enabled = ?7,"
                                      "    speed_limit_in = ?8, speed_limit_out = ?9,"
                                      "    name = ?10, block_text = ?11, allow_text = ?12,"
                                      "    period_from = ?13, period_to = ?14"
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

const char *const sqlSelectAppPathExists = "SELECT 1 FROM app WHERE path = ?1;";

const char *const sqlInsertApp =
        "INSERT INTO app(app_group_id, path, name, use_group_perm, blocked,"
        "    creat_time, end_time)"
        "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7);";

const char *const sqlInsertAppAlert = "INSERT INTO app_alert(app_id)"
                                      "  VALUES(?1);";

const char *const sqlDeleteApp = "DELETE FROM app WHERE app_id = ?1;";

const char *const sqlDeleteAppAlert = "DELETE FROM app_alert WHERE app_id = ?1;";

const char *const sqlUpdateApp = "UPDATE app"
                                 "  SET app_group_id = ?2, name = ?3, use_group_perm = ?4,"
                                 "    blocked = ?5, end_time = ?6"
                                 "  WHERE app_id = ?1;";

const char *const sqlUpdateAppName = "UPDATE app"
                                     "  SET name = ?2"
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

const char *const sqlUpdateZoneName = "UPDATE zone"
                                      "  SET name = ?2"
                                      "  WHERE zone_id = ?1;";

const char *const sqlUpdateZoneEnabled = "UPDATE zone"
                                         "  SET enabled = ?2"
                                         "  WHERE zone_id = ?1;";

const char *const sqlUpdateZoneResult = "UPDATE zone"
                                        "  SET text_checksum = ?2, bin_checksum = ?3,"
                                        "    source_modtime = ?4, last_run = ?5, last_success = ?6"
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
        Q_ASSERT(addrGroup != nullptr);

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

bool loadAppGroups(SqliteDb *db, FirewallConf &conf)
{
    SqliteStmt stmt;
    if (!db->prepare(stmt, sqlSelectAppGroups))
        return false;

    while (stmt.step() == SqliteStmt::StepRow) {
        auto appGroup = new AppGroup();

        appGroup->setId(stmt.columnInt64(0));
        appGroup->setEnabled(stmt.columnBool(1));
        appGroup->setFragmentPacket(stmt.columnInt(2));
        appGroup->setPeriodEnabled(stmt.columnBool(3));
        appGroup->setLimitInEnabled(stmt.columnBool(4));
        appGroup->setLimitOutEnabled(stmt.columnBool(5));
        appGroup->setSpeedLimitIn(quint32(stmt.columnInt(6)));
        appGroup->setSpeedLimitOut(quint32(stmt.columnInt(7)));
        appGroup->setName(stmt.columnText(8));
        appGroup->setBlockText(stmt.columnText(9));
        appGroup->setAllowText(stmt.columnText(10));
        appGroup->setPeriodFrom(stmt.columnText(11));
        appGroup->setPeriodTo(stmt.columnText(12));
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
            << appGroup->fragmentPacket() << appGroup->periodEnabled() << appGroup->limitInEnabled()
            << appGroup->limitOutEnabled() << appGroup->speedLimitIn() << appGroup->speedLimitOut()
            << appGroup->name() << appGroup->blockText() << appGroup->allowText()
            << appGroup->periodFrom() << appGroup->periodTo();

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

}

ConfManager::ConfManager(const QString &filePath, FortManager *fortManager, QObject *parent) :
    QObject(parent),
    m_fortManager(fortManager),
    m_sqliteDb(new SqliteDb(filePath)),
    m_conf(new FirewallConf(this))
{
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

void ConfManager::showErrorMessage(const QString &errorMessage)
{
    fortManager()->showErrorBox(errorMessage, tr("Configuration Error"));
}

bool ConfManager::checkResult(bool ok, bool commit)
{
    const auto errorMessage = ok ? QString() : m_sqliteDb->errorMessage();

    if (commit) {
        m_sqliteDb->endTransaction(ok);
    }

    if (!ok) {
        showErrorMessage(errorMessage);
    }

    return ok;
}

bool ConfManager::initialize()
{
    if (!m_sqliteDb->open()) {
        logCritical() << "File open error:" << m_sqliteDb->filePath() << m_sqliteDb->errorMessage();
        return false;
    }

    m_sqliteDb->execute(sqlPragmas);

    if (!m_sqliteDb->migrate(
                ":/conf/migrations", DATABASE_USER_VERSION, true, true, &migrateFunc)) {
        logCritical() << "Migration error" << m_sqliteDb->filePath();
        return false;
    }

    m_appEndTimer.start();

    return true;
}

void ConfManager::initConfToEdit()
{
    auto newConf = cloneConf(*conf(), this);
    setConfToEdit(newConf);
}

void ConfManager::setConfToEdit(FirewallConf *conf)
{
    if (m_confToEdit == conf)
        return;

    if (m_confToEdit != nullptr && m_confToEdit != m_conf) {
        m_confToEdit->deleteLater();
    }

    m_confToEdit = conf;
}

FirewallConf *ConfManager::cloneConf(const FirewallConf &conf, QObject *parent) const
{
    auto newConf = new FirewallConf(parent);

    newConf->copy(conf);

    return newConf;
}

void ConfManager::setupDefault(FirewallConf &conf) const
{
    conf.setupDefaultAddressGroups();
    conf.addDefaultAppGroup();
}

bool ConfManager::load(FirewallConf &conf)
{
    bool isNewConf = false;

    if (!loadFromDb(conf, isNewConf)) {
        showErrorMessage("Load Settings: " + m_sqliteDb->errorMessage());
        return false;
    }

    if (isNewConf) {
        setupDefault(conf);
        saveToDb(conf);
    }

    settings()->readConfIni(conf);

    return true;
}

bool ConfManager::save(FirewallConf &newConf, bool onlyFlags)
{
    if (!onlyFlags && !saveToDb(newConf))
        return false;

    if (!settings()->writeConfIni(newConf)) {
        showErrorMessage("Save Settings: " + settings()->errorMessage());
        return false;
    }

    if (m_conf != &newConf) {
        m_conf->deleteLater();
        m_conf = &newConf;

        if (m_confToEdit == m_conf) {
            setConfToEdit(nullptr);
        }
    }

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

    m_sqliteDb->beginTransaction();

    for (TaskInfo *taskInfo : taskInfos) {
        if (!saveTask(taskInfo)) {
            ok = false;
            break;
        }
    }

    return checkResult(ok, true);
}

bool ConfManager::appPathExists(const QString &appPath)
{
    return sqliteDb()->executeEx(sqlSelectAppPathExists, { appPath }).toBool();
}

bool ConfManager::addApp(const QString &appPath, const QString &appName, const QDateTime &endTime,
        qint64 groupId, bool useGroupPerm, bool blocked, bool alerted)
{
    bool ok = false;

    m_sqliteDb->beginTransaction();

    const auto vars = QVariantList()
            << groupId << appPath << appName << useGroupPerm << blocked
            << QDateTime::currentDateTime() << (!endTime.isNull() ? endTime : QVariant());

    m_sqliteDb->executeEx(sqlInsertApp, vars, 0, &ok);
    if (!ok)
        goto end;

    // Alert
    {
        const qint64 appId = m_sqliteDb->lastInsertRowid();

        if (alerted) {
            m_sqliteDb->executeEx(sqlInsertAppAlert, { appId }, 0, &ok);
        }
    }

end:
    checkResult(ok, true);

    if (ok && !endTime.isNull()) {
        m_appEndTimer.start();
    }

    if (ok && alerted) {
        emit alertedAppAdded();
    }

    return ok;
}

bool ConfManager::deleteApp(qint64 appId)
{
    bool ok = false;

    m_sqliteDb->beginTransaction();

    const auto vars = QVariantList() << appId;

    m_sqliteDb->executeEx(sqlDeleteApp, vars, 0, &ok);
    if (!ok)
        goto end;

    m_sqliteDb->executeEx(sqlDeleteAppAlert, vars, 0, &ok);

end:
    return checkResult(ok, true);
}

bool ConfManager::updateApp(qint64 appId, const QString &appName, const QDateTime &endTime,
        qint64 groupId, bool useGroupPerm, bool blocked)
{
    bool ok = false;

    m_sqliteDb->beginTransaction();

    const auto vars = QVariantList() << appId << groupId << appName << useGroupPerm << blocked
                                     << (!endTime.isNull() ? endTime : QVariant());

    m_sqliteDb->executeEx(sqlUpdateApp, vars, 0, &ok);
    if (!ok)
        goto end;

    m_sqliteDb->executeEx(sqlDeleteAppAlert, { appId }, 0, &ok);

end:
    checkResult(ok, true);

    if (ok && !endTime.isNull()) {
        m_appEndTimer.start();
    }

    return ok;
}

bool ConfManager::updateAppName(qint64 appId, const QString &appName)
{
    bool ok = false;

    const auto vars = QVariantList() << appId << appName;

    m_sqliteDb->executeEx(sqlUpdateAppName, vars, 0, &ok);

    return checkResult(ok);
}

bool ConfManager::walkApps(std::function<walkAppsCallback> func)
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
    if (!stmt.prepare(m_sqliteDb->db(), sqlSelectEndedApps))
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

        if (updateDriverUpdateApp(appPath, groupIndex, useGroupPerm, true)
                && updateApp(appId, appName, QDateTime(), groupId, useGroupPerm, true)) {
            isAppEndTimesUpdated = true;
        }
    }

    if (isAppEndTimesUpdated) {
        emit appEndTimesUpdated();
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

    m_sqliteDb->executeEx(sqlInsertZone, vars, 0, &ok);

    return checkResult(ok);
}

int ConfManager::getFreeZoneId()
{
    int zoneId = 1;

    SqliteStmt stmt;
    if (stmt.prepare(m_sqliteDb->db(), sqlSelectZoneIds)) {
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

    m_sqliteDb->beginTransaction();

    m_sqliteDb->executeEx(sqlDeleteZone, { zoneId }, 0, &ok);
    if (!ok)
        goto end;

    // Delete the Zone from Address Groups
    {
        const quint32 zoneUnMask = ~(quint32(1) << (zoneId - 1));
        m_sqliteDb->executeEx(sqlDeleteAddressGroupZone, { qint64(zoneUnMask) }, 0, &ok);
    }

end:
    return checkResult(ok, true);
}

bool ConfManager::updateZone(int zoneId, const QString &zoneName, const QString &sourceCode,
        const QString &url, const QString &formData, bool enabled, bool customUrl)
{
    bool ok = false;

    const auto vars = QVariantList()
            << zoneId << zoneName << enabled << customUrl << sourceCode << url << formData;

    m_sqliteDb->executeEx(sqlUpdateZone, vars, 0, &ok);

    return checkResult(ok);
}

bool ConfManager::updateZoneName(int zoneId, const QString &zoneName)
{
    bool ok = false;

    const auto vars = QVariantList() << zoneId << zoneName;

    m_sqliteDb->executeEx(sqlUpdateZoneName, vars, 0, &ok);

    return checkResult(ok);
}

bool ConfManager::updateZoneEnabled(int zoneId, bool enabled)
{
    bool ok = false;

    const auto vars = QVariantList() << zoneId << enabled;

    m_sqliteDb->executeEx(sqlUpdateZoneEnabled, vars, 0, &ok);

    return checkResult(ok);
}

bool ConfManager::updateZoneResult(int zoneId, const QString &textChecksum,
        const QString &binChecksum, const QDateTime &sourceModTime, const QDateTime &lastRun,
        const QDateTime &lastSuccess)
{
    bool ok = false;

    const auto vars = QVariantList()
            << zoneId << textChecksum << binChecksum << sourceModTime << lastRun << lastSuccess;

    m_sqliteDb->executeEx(sqlUpdateZoneResult, vars, 0, &ok);

    return checkResult(ok);
}

bool ConfManager::validateDriver()
{
    ConfUtil confUtil;
    QByteArray buf;

    const int verSize = confUtil.writeVersion(buf);
    if (!verSize) {
        showErrorMessage(confUtil.errorMessage());
        return false;
    }

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
        if (!loadAddressGroups(m_sqliteDb, conf.addressGroups(), count))
            return false;

        if (count == 0) {
            isNew = true;
            return true;
        }
        isNew = false;
    }

    // Load App Groups
    if (!loadAppGroups(m_sqliteDb, conf))
        return false;

    return true;
}

bool ConfManager::saveToDb(const FirewallConf &conf)
{
    bool ok = false;

    m_sqliteDb->beginTransaction();

    // Save Address Groups
    int orderIndex = -1;
    for (AddressGroup *addrGroup : conf.addressGroups()) {
        if (!saveAddressGroup(m_sqliteDb, addrGroup, ++orderIndex))
            goto end;
    }

    // Save App Groups
    orderIndex = -1;
    for (AppGroup *appGroup : conf.appGroups()) {
        if (!saveAppGroup(m_sqliteDb, appGroup, ++orderIndex))
            goto end;
    }

    ok = true;

    // Remove App Groups
    {
        Q_ASSERT(!conf.appGroups().isEmpty());
        const auto defaultAppGroupId = conf.appGroups().at(0)->id();

        for (AppGroup *appGroup : conf.removedAppGroupsList()) {
            m_sqliteDb->executeEx(sqlUpdateAppResetGroup,
                    QVariantList() << appGroup->id() << defaultAppGroupId, 0, &ok);
            if (!ok)
                goto end;

            m_sqliteDb->executeEx(sqlDeleteAppGroup, QVariantList() << appGroup->id(), 0, &ok);
            if (!ok)
                goto end;
        }

        conf.clearRemovedAppGroups();
    }

end:
    return checkResult(ok, true);
}

bool ConfManager::loadTask(TaskInfo *taskInfo)
{
    SqliteStmt stmt;
    if (!stmt.prepare(m_sqliteDb->db(), sqlSelectTaskByName))
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
    m_sqliteDb->executeEx(sql, vars, 0, &ok);
    if (!ok)
        return false;

    if (!rowExists) {
        taskInfo->setId(m_sqliteDb->lastInsertRowid());
    }
    return true;
}
