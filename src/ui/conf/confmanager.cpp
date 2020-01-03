#include "confmanager.h"

#include <QLoggingCategory>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include "../fortcommon.h"
#include "../fortsettings.h"
#include "../task/taskinfo.h"
#include "../util/dateutil.h"
#include "../util/fileutil.h"
#include "../util/osutil.h"
#include "addressgroup.h"
#include "appgroup.h"
#include "firewallconf.h"

Q_DECLARE_LOGGING_CATEGORY(CLOG_CONF_MANAGER)
Q_LOGGING_CATEGORY(CLOG_CONF_MANAGER, "fort.confManager")

#define logWarning() qCWarning(CLOG_CONF_MANAGER,)
#define logCritical() qCCritical(CLOG_CONF_MANAGER,)

#define DATABASE_USER_VERSION   3

namespace {

const char * const sqlPragmas =
        "PRAGMA journal_mode = WAL;"
        "PRAGMA locking_mode = EXCLUSIVE;"
        "PRAGMA synchronous = NORMAL;"
        ;

const char * const sqlSelectAddressGroups =
        "SELECT addr_group_id, include_all, exclude_all,"
        "    include_text, exclude_text"
        "  FROM address_group"
        "  ORDER BY order_index;"
        ;

const char * const sqlSelectAppGroups =
        "SELECT app_group_id, enabled,"
        "    fragment_packet, period_enabled,"
        "    limit_in_enabled, limit_out_enabled,"
        "    speed_limit_in, speed_limit_out,"
        "    name, block_text, allow_text,"
        "    period_from, period_to"
        "  FROM app_group"
        "  ORDER BY order_index;"
        ;

const char * const sqlSelectDefaultAppGroupId =
        "SELECT app_group_id"
        "  FROM app_group"
        "  WHERE order_index = 0;"
        ;

const char * const sqlInsertAddressGroup =
        "INSERT INTO address_group(addr_group_id, order_index,"
        "    include_all, exclude_all, include_text, exclude_text)"
        "  VALUES(?1, ?2, ?3, ?4, ?5, ?6);"
        ;

const char * const sqlUpdateAddressGroup =
        "UPDATE address_group"
        "  SET order_index = ?2,"
        "    include_all = ?3, exclude_all = ?4,"
        "    include_text = ?5, exclude_text = ?6"
        "  WHERE addr_group_id = ?1;"
        ;

const char * const sqlInsertAppGroup =
        "INSERT INTO app_group(app_group_id, order_index, enabled,"
        "    fragment_packet, period_enabled,"
        "    limit_in_enabled, limit_out_enabled,"
        "    speed_limit_in, speed_limit_out,"
        "    name, block_text, allow_text,"
        "    period_from, period_to)"
        "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14);"
        ;

const char * const sqlUpdateAppGroup =
        "UPDATE app_group"
        "  SET order_index = ?2, enabled = ?3,"
        "    fragment_packet = ?4, period_enabled = ?5,"
        "    limit_in_enabled = ?6, limit_out_enabled = ?7,"
        "    speed_limit_in = ?8, speed_limit_out = ?9,"
        "    name = ?10, block_text = ?11, allow_text = ?12,"
        "    period_from = ?13, period_to = ?14"
        "  WHERE app_group_id = ?1;"
        ;

const char * const sqlDeleteAppGroup =
        "DELETE FROM app_group"
        "  WHERE app_group_id = ?1;"
        ;

const char * const sqlPurgeAppByGroupId =
        "UPDATE app"
        "  SET app_group_id = 0"
        "  WHERE app_group_id = ?1;"
        ;

const char * const sqlSelectTaskByName =
        "SELECT task_id, enabled, interval_hours,"
        "    last_run, last_success, data"
        "  FROM task"
        "  WHERE name = ?1;"
        ;

const char * const sqlInsertTask =
        "INSERT INTO task(task_id, name, enabled, interval_hours,"
        "    last_run, last_success, data)"
        "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7);"
        ;

const char * const sqlUpdateTask =
        "UPDATE task"
        "  SET name = ?2, enabled = ?3, interval_hours = ?4,"
        "    last_run = ?5, last_success = ?6, data = ?7"
        "  WHERE task_id = ?1;"
        ;

const char * const sqlSelectAppCount =
        "SELECT count(*) FROM app;"
        ;

const char * const sqlSelectAppByIndex =
        "SELECT t.app_id, t.app_group_id,"
        "    g.name as app_group_name,"
        "    t.path, t.blocked,"
        "    (alert.app_id IS NOT NULL) as alerted,"
        "    t.end_time"
        "  FROM app t"
        "    JOIN app_group g ON g.app_group_id = t.app_group_id"
        "    LEFT JOIN app_alert alert ON alert.app_id = t.app_id"
        "  LIMIT 1 OFFSET ?1;"
        ;

const char * const sqlInsertApp =
        "INSERT INTO app(app_group_id, path, blocked,"
        "    creat_time, end_time)"
        "  VALUES(?1, ?2, ?3, ?4, ?5);"
        ;

const char * const sqlInsertAppAlert =
        "INSERT INTO app_alert(app_id)"
        "  VALUES(?1);"
        ;

}

ConfManager::ConfManager(const QString &filePath,
                         FortSettings *fortSettings,
                         QObject *parent) :
    QObject(parent),
    m_fortSettings(fortSettings),
    m_sqliteDb(new SqliteDb(filePath))
{
}

ConfManager::~ConfManager()
{
    delete m_sqliteDb;
}

void ConfManager::setErrorMessage(const QString &errorMessage)
{
    if (m_errorMessage != errorMessage) {
        m_errorMessage = errorMessage;
        emit errorMessageChanged();
    }
}

bool ConfManager::initialize()
{
    if (!m_sqliteDb->open()) {
        logCritical() << "File open error:"
                      << m_sqliteDb->filePath()
                      << m_sqliteDb->errorMessage();
        return false;
    }

    m_sqliteDb->execute(sqlPragmas);

    if (!m_sqliteDb->migrate(":/conf/migrations",
                             DATABASE_USER_VERSION, true, true)) {
        logCritical() << "Migration error"
                      << m_sqliteDb->filePath();
        return false;
    }

    return true;
}

FirewallConf *ConfManager::cloneConf(const FirewallConf &conf,
                                     QObject *parent)
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
    bool isNewConf = true;

    if (!loadFromDb(conf, isNewConf)) {
        setErrorMessage(m_sqliteDb->errorMessage());
        return false;
    }

    // COMPAT: v3.0.0
    if (isNewConf) {
        if (!m_fortSettings->readConf(conf, isNewConf)) {
            setErrorMessage(m_fortSettings->errorMessage());
            return false;
        }

        if (!isNewConf) {
            for (AppGroup *appGroup : conf.appGroups()) {
                appGroup->setBlockText(migrateAppsText(appGroup->blockText()));
                appGroup->setAllowText(migrateAppsText(appGroup->allowText()));
            }
        }
    }

    if (isNewConf) {
        setupDefault(conf);
    }

    m_fortSettings->readConfIni(conf);

    return true;
}

bool ConfManager::save(const FirewallConf &conf, bool onlyFlags)
{
    if (!onlyFlags && !saveToDb(conf))
        return false;

    if (!m_fortSettings->writeConfIni(conf)) {
        setErrorMessage(m_fortSettings->errorMessage());
        return false;
    }

    // Remove old JSON config.
    FileUtil::removeFile(m_fortSettings->confOldFilePath());
    FileUtil::removeFile(m_fortSettings->confBackupFilePath());

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

    if (!ok) {
        setErrorMessage(m_sqliteDb->errorMessage());
    }

    m_sqliteDb->endTransaction(ok);

    return ok;
}

int ConfManager::appCount()
{
    SqliteStmt stmt;
    if (!stmt.prepare(m_sqliteDb->db(), sqlSelectAppCount)
            || stmt.step() != SqliteStmt::StepRow)
        return 0;

    return stmt.columnInt(0);
}

bool ConfManager::getAppByIndex(bool &blocked, bool &alerted,
                                qint64 &appId, qint64 &appGroupId,
                                QString &appGroupName, QString &appPath,
                                QDateTime &endTime, int row)
{
    SqliteStmt stmt;
    if (!stmt.prepare(m_sqliteDb->db(), sqlSelectAppByIndex))
        return false;

    stmt.bindInt(1, row);

    if (stmt.step() != SqliteStmt::StepRow)
        return false;

    appId = stmt.columnInt64(0);
    appGroupId = stmt.columnInt64(1);
    appGroupName = stmt.columnText(2);
    appPath = stmt.columnText(3);
    blocked = stmt.columnBool(4);
    alerted = stmt.columnBool(5);
    endTime = stmt.columnDateTime(6);

    return true;
}

qint64 ConfManager::getDefaultAppGroupId()
{
    return m_sqliteDb->executeEx(sqlSelectDefaultAppGroupId).toLongLong();
}

bool ConfManager::addApp(const QString &appPath,
                         const QDateTime &endTime,
                         qint64 appGroupId,
                         bool blocked, bool alerted)
{
    bool ok = false;

    m_sqliteDb->beginTransaction();

    if (appGroupId == 0) {
        appGroupId = getDefaultAppGroupId();
    }

    const QVariantList vars = QVariantList()
            << appGroupId
            << appPath
            << blocked
            << QDateTime::currentDateTime()
            << endTime
               ;

    m_sqliteDb->executeEx(sqlInsertApp, vars, 0, &ok);
    if (!ok) goto end;

    // Alert
    {
        const qint64 appId = m_sqliteDb->lastInsertRowid();

        if (alerted) {
            m_sqliteDb->executeEx(sqlInsertAppAlert, {appId}, 0, &ok);
        }
    }

end:
    if (!ok) {
        setErrorMessage(m_sqliteDb->errorMessage());
    }

    m_sqliteDb->endTransaction(ok);

    return ok;
}

bool ConfManager::loadFromDb(FirewallConf &conf, bool &isNew)
{
    // Load Address Groups
    {
        SqliteStmt stmt;
        if (!stmt.prepare(m_sqliteDb->db(), sqlSelectAddressGroups))
            return false;

        int index = 0;
        while (stmt.step() == SqliteStmt::StepRow) {
            auto addrGroup = conf.addressGroups().at(index);
            Q_ASSERT(addrGroup != nullptr);

            addrGroup->setId(stmt.columnInt64(0));
            addrGroup->setIncludeAll(stmt.columnBool(1));
            addrGroup->setExcludeAll(stmt.columnBool(2));
            addrGroup->setIncludeText(stmt.columnText(3));
            addrGroup->setExcludeText(stmt.columnText(4));

            if (++index > 1)
                break;
        }

        if (index == 0) {
            isNew = true;
            return true;
        }
        isNew = false;
    }

    // Load App Groups
    {
        SqliteStmt stmt;
        if (!stmt.prepare(m_sqliteDb->db(), sqlSelectAppGroups))
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

            conf.addAppGroup(appGroup);
        }
    }

    return true;
}

bool ConfManager::saveToDb(const FirewallConf &conf)
{
    bool ok = true;

    m_sqliteDb->beginTransaction();

    // Save Address Groups
    int orderIndex = -1;
    for (AddressGroup *addrGroup : conf.addressGroups()) {
        ++orderIndex;

        const bool rowExists = (addrGroup->id() != 0);
        if (!addrGroup->edited() && rowExists)
            continue;

        const QVariantList vars = QVariantList()
                << (rowExists ? addrGroup->id() : QVariant())
                << orderIndex++
                << addrGroup->includeAll()
                << addrGroup->excludeAll()
                << addrGroup->includeText()
                << addrGroup->excludeText()
                   ;

        const char *sql = rowExists ? sqlUpdateAddressGroup : sqlInsertAddressGroup;

        m_sqliteDb->executeEx(sql, vars, 0, &ok);
        if (!ok) goto end;

        if (!rowExists) {
            addrGroup->setId(m_sqliteDb->lastInsertRowid());
        }
        addrGroup->setEdited(false);
    }

    // Save App Groups
    orderIndex = -1;
    for (AppGroup *appGroup : conf.appGroups()) {
        ++orderIndex;

        const bool rowExists = (appGroup->id() != 0);
        if (!appGroup->edited() && rowExists)
            continue;

        const QVariantList vars = QVariantList()
                << (rowExists ? appGroup->id() : QVariant())
                << orderIndex
                << appGroup->enabled()
                << appGroup->fragmentPacket()
                << appGroup->periodEnabled()
                << appGroup->limitInEnabled()
                << appGroup->limitOutEnabled()
                << appGroup->speedLimitIn()
                << appGroup->speedLimitOut()
                << appGroup->name()
                << appGroup->blockText()
                << appGroup->allowText()
                << appGroup->periodFrom()
                << appGroup->periodTo()
                   ;

        const char *sql = rowExists ? sqlUpdateAppGroup : sqlInsertAppGroup;

        m_sqliteDb->executeEx(sql, vars, 0, &ok);
        if (!ok) goto end;

        if (!rowExists) {
            appGroup->setId(m_sqliteDb->lastInsertRowid());
        }
        appGroup->setEdited(false);
    }

    // Remove App Groups
    for (AppGroup *appGroup : conf.removedAppGroupsList()) {
        m_sqliteDb->executeEx(sqlPurgeAppByGroupId,
                              QVariantList() << appGroup->id(), 0, &ok);
        if (!ok) goto end;

        m_sqliteDb->executeEx(sqlDeleteAppGroup,
                              QVariantList() << appGroup->id(), 0, &ok);
        if (!ok) goto end;
    }

    conf.clearRemovedAppGroups();

 end:
    if (!ok) {
        setErrorMessage(m_sqliteDb->errorMessage());
    }

    m_sqliteDb->endTransaction(ok);

    return ok;
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

    const QVariantList vars = QVariantList()
            << (rowExists ? taskInfo->id() : QVariant())
            << taskInfo->name()
            << taskInfo->enabled()
            << taskInfo->intervalHours()
            << taskInfo->lastRun()
            << taskInfo->lastSuccess()
            << taskInfo->data()
               ;

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

QString ConfManager::migrateAppsText(const QString &text)
{
    if (text.isEmpty())
        return QString();

    QStringList list;

    const QLatin1String systemPath("System");

    for (const auto &line : text.splitRef('\n')) {
        QString fixedLine = line.toString();

        const auto lineTrimmed = line.trimmed();
        if (!(lineTrimmed.isEmpty()
              || lineTrimmed.startsWith('#'))) {

            QStringRef path = lineTrimmed;
            bool addQuotes = false;
            if (path.startsWith('"') && path.endsWith('"')) {
                path = path.mid(1, path.size() - 2);
                addQuotes = true;
            }

            if (!path.isEmpty()
                    && QStringRef::compare(path, systemPath, Qt::CaseInsensitive) != 0
                    && !path.endsWith(".exe")) {
                fixedLine = path + "**";
                if (addQuotes) {
                    fixedLine = '"' + fixedLine + '"';
                }
            }
        }

        list.append(fixedLine);
    }

    return list.join('\n');
}
