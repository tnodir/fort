#include "activitymanager.h"

#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/dbvar.h>
#include <sqlite/sqlitestmt.h>

#include <common/fortlog.h>
#include <log/logentryconn.h>
#include <log/logentryflowclosed.h>
#include <log/logentryflowtraf.h>
#include <log/logentryprocnew.h>
#include <log/logentrystattraf.h>
#include <util/fileutil.h>
#include <util/net/netutil.h>
#include <util/net/processipkey.h>

namespace {

const QLoggingCategory LC("activity");

inline constexpr int DATABASE_USER_VERSION = 1;

inline constexpr qint64 hostWaitMsecs = 5 * 1000;

const char *const sqlInsertFlow =
        "INSERT OR IGNORE INTO flow(flow_id, pid, terminated, inherited, inbound, loopback,"
        "    local_net, reason, ip_proto, zone_id, rule_id, local_port, remote_port,"
        "    local_ip, remote_ip, local_ip6, remote_ip6, remote_host, inherit_path, conn_time)"
        "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17,"
        "    ?18, ?19, ?20);";

const char *const sqlReplaceProc =
        "INSERT OR REPLACE INTO proc(pid, app_id, parent_pid, path, parent_path)"
        "  VALUES(?1, ?2, ?3, ?4, ?5);";

const char *const sqlUpdateProcPath =
        "UPDATE proc SET path = ?2 WHERE pid = ?1 AND path IS NOT ?2;";

const char *const sqlUpdateProcApp =
        "UPDATE proc SET app_id = ?2 WHERE pid = ?1 AND app_id IS NOT ?2;";

const char *const sqlUpdateFlowReason =
        "UPDATE flow SET terminated = ?2, inherited = ?3, reason = ?4, zone_id = ?5,"
        "    rule_id = ?6, inherit_path = ?7"
        "  WHERE flow_id = ?1"
        "    AND (terminated IS NOT ?2 OR inherited IS NOT ?3 OR reason IS NOT ?4"
        "      OR zone_id IS NOT ?5 OR rule_id IS NOT ?6 OR inherit_path IS NOT ?7);";

const char *const sqlUpdateFlowTraf = "UPDATE flow"
                                      "  SET in_bytes = in_bytes + ?2, out_bytes = out_bytes + ?3,"
                                      "    in_speed = ?4, out_speed = ?5, updated_tick = ?6"
                                      "  WHERE flow_id = ?1;";

const char *const sqlUpdateFlowHost4 =
        "UPDATE flow SET remote_host = ?1"
        "  WHERE pid = ?2 AND remote_ip = ?3 AND remote_host IS NULL;";

const char *const sqlUpdateFlowHost6 =
        "UPDATE flow SET remote_host = ?1"
        "  WHERE pid = ?2 AND remote_ip6 = ?3 AND remote_host IS NULL;";

const char *const sqlDeleteFlow = "DELETE FROM flow WHERE flow_id = ?1;";

const char *const sqlDeleteProcFlows = "DELETE FROM flow WHERE pid = ?1;";

const char *const sqlDeleteEmptyProcs = "DELETE FROM proc"
                                        "  WHERE NOT EXISTS (SELECT 1 FROM flow"
                                        "    WHERE flow.pid = proc.pid);";

const char *const sqlClearIdleSpeeds = "UPDATE flow SET in_speed = 0, out_speed = 0"
                                       "  WHERE (in_speed <> 0 OR out_speed <> 0)"
                                       "    AND updated_tick < ?1;";

// The speed of the bytes got in the driver's tick
qint64 tickSpeed(quint32 bytes)
{
    return qint64(bytes) * 1000 / FORT_LOG_TIMER_PERIOD;
}

QVariantList flowVars(const LogEntryConn &entry)
{
    const bool isIPv6 = entry.isIPv6();

    return {
        qint64(entry.flowId()),
        entry.pid(),
        entry.terminated(),
        entry.inherited(),
        entry.inbound(),
        entry.loopback(),
        entry.localNet(),
        entry.reason(),
        entry.ipProto(),
        DbVar::nullable(entry.zoneId()),
        DbVar::nullable(entry.ruleId()),
        entry.localPort(),
        entry.remotePort(),
        isIPv6 ? QVariant() : QVariant(qint32(entry.localIp4())),
        isIPv6 ? QVariant() : QVariant(qint32(entry.remoteIp4())),
        isIPv6 ? QVariant(entry.localIp6View().toByteArray()) : QVariant(),
        isIPv6 ? QVariant(entry.remoteIp6View().toByteArray()) : QVariant(),
        DbVar::nullable(entry.remoteHost()),
        DbVar::nullable(entry.inheritPath()),
        entry.connTime(),
    };
}

// The reason with its details
QVariantList flowReasonVars(const LogEntryConn &entry)
{
    return {
        qint64(entry.flowId()),
        entry.terminated(),
        entry.inherited(),
        entry.reason(),
        DbVar::nullable(entry.zoneId()),
        DbVar::nullable(entry.ruleId()),
        DbVar::nullable(entry.inheritPath()),
    };
}

}

ActivityManager::ActivityManager(const QString &filePath, QObject *parent, quint32 openFlags) :
    QObject(parent), m_sqliteDb(new SqliteDb(filePath, openFlags)), m_hostWaits(hostWaitMsecs)
{
}

void ActivityManager::setUp()
{
    // The live flows don't survive the restart: recreate the DB by the current schema
    if (!sqliteDb()->isReadOnly()) {
        FileUtil::removeFilesByPrefix(sqliteDb()->filePath());
    }

    if (!setupDb())
        return;

    if (!sqliteDb()->isReadOnly()) {
        sqliteDb()->setSynchronous(SqliteDb::SyncOff);

        // The old file may stay opened by a client: not removed
        clear();
    }
}

void ActivityManager::tearDown()
{
    clear();
}

bool ActivityManager::setupDb()
{
    SqliteDb::MigrateOptions opt = {
        .sqlDir = ":/activity/migrations",
        .version = DATABASE_USER_VERSION,
        .recreate = true,
    };

    return sqliteDb()->openAndMigrate(opt);
}

bool ActivityManager::beginChanges()
{
    if (m_inChanges)
        return true;

    // The DB's error is logged by its opening: not by each log entry
    if (!sqliteDb()->isOpened())
        return false;

    if (!beginWriteTransaction()) {
        qCWarning(LC) << "Begin transaction error:" << sqliteDb()->errorMessage();
        return false;
    }

    m_inChanges = true;

    return true;
}

void ActivityManager::logConn(
        const LogEntryConn &entry, quint32 parentPid, const QString &parentPath)
{
    if (!beginChanges())
        return;

    // Logged again (after a blocked reauthorization): the flow is kept, its reason and Program may
    // be changed
    if (!insertFlow(entry)) {
        updateFlowReason(entry);
        updateProcApp(entry);
        return;
    }

    replaceProc(entry, parentPid, parentPath);

    waitConnHost(entry);
}

bool ActivityManager::doneChange(SqliteStmt *stmt)
{
    if (!sqliteDb()->done(stmt))
        return false;

    m_changed = true;

    return true;
}

bool ActivityManager::doneChange(const char *sql, const QVariantList &vars)
{
    SqliteStmt *stmt = sqliteDb()->stmt(sql);

    return stmt->bindVars(vars) && doneChange(stmt);
}

bool ActivityManager::insertFlow(const LogEntryConn &entry)
{
    return doneChange(sqlInsertFlow, flowVars(entry));
}

void ActivityManager::replaceProc(
        const LogEntryConn &entry, quint32 parentPid, const QString &parentPath)
{
    doneChange(sqlReplaceProc,
            { entry.pid(), DbVar::nullable(entry.appId()), DbVar::nullable(parentPid), entry.path(),
                    DbVar::nullable(parentPath) });
}

void ActivityManager::waitConnHost(const LogEntryConn &entry)
{
    if (!entry.remoteHost().isEmpty())
        return;

    m_hostWaits.insert(entry.remoteIpKey(), true, QDeadlineTimer(hostWaitMsecs));
}

void ActivityManager::updateFlowReason(const LogEntryConn &entry)
{
    doneChange(sqlUpdateFlowReason, flowReasonVars(entry));
}

void ActivityManager::updateProcApp(const LogEntryConn &entry)
{
    doneChange(sqlUpdateProcApp, { entry.pid(), DbVar::nullable(entry.appId()) });
}

void ActivityManager::logProcNew(const LogEntryProcNew &entry, const QString &path)
{
    if (!beginChanges())
        return;

    // The process of a known flow is logged again: its name is changed
    doneChange(sqlUpdateProcPath, { entry.pid(), path });
}

void ActivityManager::logFlowTraf(const LogEntryFlowTraf &entry)
{
    updateFlowsTraf(entry);

    if (entry.isTickEnd()) {
        clearIdleSpeeds();
    }
}

void ActivityManager::updateFlowsTraf(const LogEntryFlowTraf &entry)
{
    const int flowCount = entry.flowCount();
    if (flowCount == 0 || !beginChanges())
        return;

    SqliteStmt *stmt = sqliteDb()->stmt(sqlUpdateFlowTraf);

    for (int i = 0; i < flowCount; ++i) {
        const FORT_FLOW_STAT flowStat = entry.flowStat(i);
        const FORT_TRAF &traf = flowStat.traf;

        stmt->bindInt64(1, qint64(flowStat.flow_id));
        stmt->bindInt64(2, qint64(traf.in_bytes));
        stmt->bindInt64(3, qint64(traf.out_bytes));
        stmt->bindInt64(4, tickSpeed(traf.in_bytes));
        stmt->bindInt64(5, tickSpeed(traf.out_bytes));
        stmt->bindInt64(6, m_tick);

        // The flow's CONN may be lost or logged after its bytes by a reauthorization: ignored
        if (doneChange(stmt)) {
            m_tickHasSpeeds = true;
        }
    }
}

void ActivityManager::logFlowClosed(const LogEntryFlowClosed &entry)
{
    closeFlows(sqlDeleteFlow, entry.flowId());
}

void ActivityManager::logStatTraf(const LogEntryStatTraf &entry)
{
    const int procCount = entry.procCount();

    for (int i = 0; i < procCount; ++i) {
        const LogEntryStatTraf::ProcTraf procTraf = entry.procTraf(i);

        // The process's flows are closed: their FLOW_CLOSED may be lost by the log's overflow
        if (procTraf.inactive) {
            closeFlows(sqlDeleteProcFlows, procTraf.pid);
        }
    }
}

void ActivityManager::closeFlows(const char *sql, quint64 id)
{
    if (!beginChanges())
        return;

    if (doneChange(sql, { qint64(id) })) {
        m_hasEmptyProcesses = true;
    }
}

void ActivityManager::updateConnHost(const ProcessIpKey &key, const QString &name)
{
    // Most of the names are resolved before the connections: only a flow logged without it waits
    if (name.isEmpty() || !m_hostWaits.take(key).has_value())
        return;

    if (!beginChanges())
        return;

    updateFlowsHost(key, name);

    // Out of the log buffer's processing: committed at once
    commitChanges();
}

void ActivityManager::updateFlowsHost(const ProcessIpKey &key, const QString &name)
{
    const bool isIPv6 = (key.isIPv6 != 0);

    const QVariant ipVar = isIPv6 ? QVariant(NetUtil::ip6ToArrayView(key.ip.v6).toByteArray())
                                  : QVariant(qint32(key.ip.v4));

    doneChange(isIPv6 ? sqlUpdateFlowHost6 : sqlUpdateFlowHost4, { name, key.pid, ipVar });
}

void ActivityManager::logTime()
{
    ++m_tick;
}

void ActivityManager::commitChanges()
{
    if (!m_inChanges)
        return;

    removeEmptyProcesses();

    const bool changed = m_changed;

    m_inChanges = false;
    m_changed = false;

    // A failed commit leaves the transaction opened: the next ones couldn't begin
    if (!sqliteDb()->commitTransaction()) {
        qCWarning(LC) << "Commit transaction error:" << sqliteDb()->errorMessage();

        sqliteDb()->rollbackTransaction();
        return;
    }

    if (changed) {
        emit activityChanged();
    }
}

void ActivityManager::removeEmptyProcesses()
{
    if (!m_hasEmptyProcesses)
        return;

    m_hasEmptyProcesses = false;

    sqliteDb()->done(sqliteDb()->stmt(sqlDeleteEmptyProcs));
}

void ActivityManager::clearIdleSpeeds()
{
    // Only the previous tick's speeds may stay: the older ones are cleared at its end
    const bool hasSpeeds = m_hasSpeeds;

    m_hasSpeeds = m_tickHasSpeeds;
    m_tickHasSpeeds = false;

    if (!hasSpeeds || !beginChanges())
        return;

    SqliteStmt *stmt = sqliteDb()->stmt(sqlClearIdleSpeeds);

    stmt->bindInt64(1, m_tick);

    doneChange(stmt);
}

void ActivityManager::clear()
{
    if (sqliteDb()->isReadOnly())
        return;

    if (!beginChanges())
        return;

    m_hasEmptyProcesses = false;

    DbQuery(sqliteDb()).sql("DELETE FROM flow;").executeOk();
    DbQuery(sqliteDb()).sql("DELETE FROM proc;").executeOk();

    m_changed = true;

    commitChanges();
}
