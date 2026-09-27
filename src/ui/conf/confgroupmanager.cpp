#include "confgroupmanager.h"

#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/group.h>
#include <driver/drivermanager.h>
#include <fortglobal.h>
#include <util/bitutil.h>
#include <util/conf/confbuffer.h>
#include <util/conf/confutil.h>
#include <util/dateutil.h>
#include <util/ioc/ioccontainer.h>

#include "confmanager.h"

namespace {

const QLoggingCategory LC("confGroup");

inline constexpr int GROUP_PERIODS_UPDATE_INTERVAL = 60 * 1000; // 1 minute

#define SELECT_GROUP_FIELDS                                                                        \
    "    t.group_id,"                                                                              \
    "    t.enabled,"                                                                               \
    "    t.exclusive,"                                                                             \
    "    t.period_enabled,"                                                                        \
    "    t.name,"                                                                                  \
    "    t.notes,"                                                                                 \
    "    t.period_from,"                                                                           \
    "    t.period_to,"                                                                             \
    "    t.rule_id,"                                                                               \
    "    t.mod_time"

const char *const sqlSelectGroups = "SELECT" SELECT_GROUP_FIELDS "  FROM app_group2 t"
                                    "  ORDER BY t.group_id;";

const char *const sqlInsertGroup = "INSERT INTO app_group2(group_id, name, notes, enabled,"
                                   "    exclusive, period_enabled, period_from, period_to,"
                                   "    rule_id, mod_time)"
                                   "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10);";

const char *const sqlUpdateGroup = "UPDATE app_group2"
                                   "  SET name = ?2, notes = ?3, enabled = ?4, exclusive = ?5,"
                                   "    period_enabled = ?6, period_from = ?7, period_to = ?8,"
                                   "    rule_id = ?9, mod_time = ?10"
                                   "  WHERE group_id = ?1;";

const char *const sqlSelectGroupNameById = "SELECT name FROM app_group2 WHERE group_id = ?1;";

const char *const sqlSelectGroupIds = "SELECT group_id FROM app_group2"
                                      "  WHERE group_id <= ?1 ORDER BY group_id;";

const char *const sqlDeleteGroup = "DELETE FROM app_group2 WHERE group_id = ?1;";

const char *const sqlDeleteAppGroup = "UPDATE app"
                                      "  SET groups_mask = groups_mask & ~?1"
                                      "  WHERE (groups_mask & ?1) <> 0;";

const char *const sqlUpdateGroupName = "UPDATE app_group2 SET name = ?2 WHERE group_id = ?1;";

const char *const sqlUpdateGroupEnabled = "UPDATE app_group2 SET enabled = ?2 WHERE group_id = ?1;";

const char *const sqlSelectAnyGroupPeriod = "SELECT 1 FROM app_group2"
                                            "  WHERE enabled = 1 AND period_enabled = 1"
                                            "  LIMIT 1;";

bool driverWriteGroups(ConfBuffer &confBuf, bool onlyFlags = false)
{
    if (confBuf.hasError()) {
        qCWarning(LC) << "Driver config error:" << confBuf.errorMessage();
        return false;
    }

    auto driverManager = IoC<DriverManager>();
    if (!driverManager->writeGroups(confBuf.buffer(), onlyFlags)) {
        qCWarning(LC) << "Update driver error:" << driverManager->errorMessage();
        return false;
    }

    return true;
}

}

ConfGroupManager::ConfGroupManager(QObject *parent) : ConfManagerBase(parent)
{
    setupPeriodsTimer();
    setupGroupNamesCache();
}

void ConfGroupManager::setUp()
{
    auto confManager = Fort::dependency<ConfManager>();

    // The imported DB may reuse the ids
    connect(confManager, &ConfManager::imported, this, &ConfGroupManager::clearGroupNamesCache);
}

QString ConfGroupManager::groupNameById(quint8 groupId)
{
    if (groupId == 0)
        return {};

    QString name = m_groupNamesCache.value(groupId);

    if (name.isEmpty()) {
        name = DbQuery(sqliteDb())
                       .sql(sqlSelectGroupNameById)
                       .vars({ groupId })
                       .execute()
                       .toString();

        m_groupNamesCache.insert(groupId, name);
    }

    return name;
}

QStringList ConfGroupManager::groupNamesByMask(quint32 groupsMask)
{
    QStringList list;

    while (groupsMask != 0) {
        const int groupIndex = BitUtil::bitScanForward(groupsMask);
        if (Q_UNLIKELY(groupIndex == -1))
            break;

        const quint8 groupId = groupIndex + 1;

        list << groupNameById(groupId);

        groupsMask ^= (1u << groupIndex);
    }

    return list;
}

bool ConfGroupManager::addOrUpdateGroup(Group &group)
{
    bool ok = true;

    beginWriteTransaction();

    const bool isNew = (group.groupId == 0);
    if (isNew) {
        group.groupId = DbQuery(sqliteDb(), &ok)
                                .sql(sqlSelectGroupIds)
                                .vars({ ConfUtil::groupMaxCount() })
                                .getFreeId(/*maxId=*/ConfUtil::groupMaxCount());
    }

    if (ok) {
        const QVariantList vars = {
            group.groupId,
            group.groupName,
            group.notes,
            group.enabled,
            group.exclusive,
            group.periodEnabled,
            group.periodFrom,
            group.periodTo,
            group.ruleId,
            DateUtil::now(),
        };

        DbQuery(sqliteDb(), &ok)
                .sql(isNew ? sqlInsertGroup : sqlUpdateGroup)
                .vars(vars)
                .executeOk();
    }

    endTransaction(ok);

    if (!ok)
        return false;

    if (isNew) {
        emit groupAdded();
    } else {
        emit groupUpdated();
    }

    updateDriverGroups();

    return true;
}

bool ConfGroupManager::deleteGroup(quint8 groupId)
{
    if (groupId == 0 || groupId > ConfUtil::groupMaxCount())
        return false; // out of range Group

    bool ok = false;

    beginWriteTransaction();

    if (DbQuery(sqliteDb(), &ok).sql(sqlDeleteGroup).vars({ groupId }).executeOk()) {
        const quint32 groupBit = (quint32(1) << (groupId - 1));
        const QVariantList vars = { groupBit };

        // Delete the Group from Programs
        DbQuery(sqliteDb(), &ok).sql(sqlDeleteAppGroup).vars(vars).executeOk();
    }

    endTransaction(ok);

    if (ok) {
        emit groupRemoved(groupId);

        updateDriverGroups();
    }

    return ok;
}

bool ConfGroupManager::updateGroupName(quint8 groupId, const QString &groupName)
{
    bool ok = false;

    beginWriteTransaction();

    const QVariantList vars = { groupId, groupName };

    DbQuery(sqliteDb(), &ok).sql(sqlUpdateGroupName).vars(vars).executeOk();

    endTransaction(ok);

    if (ok) {
        emit groupUpdated();
    }

    return ok;
}

bool ConfGroupManager::updateGroupEnabled(quint8 groupId, bool enabled)
{
    bool ok = false;

    beginWriteTransaction();

    const QVariantList vars = { groupId, enabled };

    DbQuery(sqliteDb(), &ok).sql(sqlUpdateGroupEnabled).vars(vars).executeOk();

    endTransaction(ok);

    if (ok) {
        emit groupUpdated();

        updateDriverGroupFlags(); // the Group's period may keep it inactive
    }

    return ok;
}

bool ConfGroupManager::walkGroups(const std::function<walkGroupsCallback> &func) const
{
    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sqlSelectGroups).prepare(stmt))
        return false;

    while (stmt.step() == SqliteStmt::StepRow) {
        Group group;
        fillGroup(group, stmt);

        if (!func(group))
            return false;
    }

    return true;
}

void ConfGroupManager::updateDriverGroups()
{
    stopPeriodsTimer();

    const quint32 activeMask = activeGroupsMask();

    ConfBuffer confBuf;

    confBuf.writeGroups(*this, activeMask);

    if (driverWriteGroups(confBuf)) {
        m_driverActiveMask = activeMask;
    }

    startPeriodsTimer();
}

void ConfGroupManager::updateDriverGroupFlags()
{
    stopPeriodsTimer();

    const quint32 activeMask = activeGroupsMask();

    if (activeMask != m_driverActiveMask) {
        ConfBuffer confBuf;

        confBuf.writeGroupFlags(activeMask);

        if (driverWriteGroups(confBuf, /*onlyFlags=*/true)) {
            m_driverActiveMask = activeMask;
        }
    }

    startPeriodsTimer();
}

quint32 ConfGroupManager::activeGroupsMask() const
{
    const QTime now = DateUtil::currentTime();

    quint32 activeMask = 0;

    walkGroups([&](Group &group) -> bool {
        if (Q_UNLIKELY(group.groupId <= 0 || group.groupId > ConfUtil::groupMaxCount()))
            return true; // skip an out of range Group

        if (group.isActive(now)) {
            activeMask |= (quint32(1) << (group.groupId - 1));
        }

        return true;
    });

    return activeMask;
}

void ConfGroupManager::setupPeriodsTimer()
{
    m_periodsTimer.setSingleShot(true);
    m_periodsTimer.setTimerType(Qt::PreciseTimer); // not earlier than the next minute

    connect(&m_periodsTimer, &QTimer::timeout, this, &ConfGroupManager::updateDriverGroupFlags);
}

void ConfGroupManager::startPeriodsTimer()
{
    const bool anyPeriodEnabled =
            DbQuery(sqliteDb()).sql(sqlSelectAnyGroupPeriod).execute().toBool();

    if (anyPeriodEnabled) {
        // Wake up at the start of the next minute: the periods are in "hh:mm"
        const int msecs = DateUtil::currentTime().msecsSinceStartOfDay();
        m_periodsTimer.start(GROUP_PERIODS_UPDATE_INTERVAL - msecs % GROUP_PERIODS_UPDATE_INTERVAL);
    }
}

void ConfGroupManager::stopPeriodsTimer()
{
    m_periodsTimer.stop();
}

void ConfGroupManager::setupGroupNamesCache()
{
    connect(this, &ConfGroupManager::groupRemoved, this, &ConfGroupManager::clearGroupNamesCache);
    connect(this, &ConfGroupManager::groupUpdated, this, &ConfGroupManager::clearGroupNamesCache);
}

void ConfGroupManager::clearGroupNamesCache()
{
    m_groupNamesCache.clear();
}

void ConfGroupManager::fillGroup(Group &group, const SqliteStmt &stmt)
{
    group.groupId = stmt.columnInt64(0);
    group.enabled = stmt.columnBool(1);
    group.exclusive = stmt.columnBool(2);
    group.periodEnabled = stmt.columnBool(3);
    group.groupName = stmt.columnText(4);
    group.notes = stmt.columnText(5);
    group.periodFrom = stmt.columnText(6);
    group.periodTo = stmt.columnText(7);
    group.ruleId = stmt.columnInt64(8);
    group.modTime = stmt.columnDateTime(9);
}
