#include "activitymanager.h"

#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/dbvar.h>
#include <sqlite/sqlitestmt.h>

#include <common/fortlog.h>
#include <dns/dnsclientmanager.h>
#include <fortglobal.h>
#include <log/logentryconn.h>
#include <log/logentryflowclosed.h>
#include <log/logentryflowtraf.h>
#include <log/logentryprocexit.h>
#include <log/logentryprocnew.h>
#include <log/logentryprocstart.h>
#include <log/logentrystattraf.h>
#include <util/fileutil.h>
#include <util/net/netdbutil.h>
#include <util/net/processipkey.h>
#include <util/osutil.h>
#include <util/processinfo.h>

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

// The parent's path is valid only by the creation times' order, an unknown time is trusted: the
// ids are reused. The enumerated process may be logged again: kept. The row added by the process's
// CONN (without the time: the conf is set before the enumeration on the master's restart) keeps its
// Program and path
const char *const sqlUpsertProcStart =
        "INSERT INTO proc(pid, parent_pid, create_time, path, cmdline, parent_path)"
        "  VALUES(?1, ?2, ?3, ?4, ?5,"
        "    (SELECT path FROM proc WHERE pid = ?2"
        "      AND (create_time IS NULL OR ?3 IS NULL OR create_time < ?3)))"
        "  ON CONFLICT(pid) DO UPDATE SET app_id = IIF(create_time IS NULL, app_id, NULL),"
        "    parent_pid = ?2, create_time = ?3, path = IIF(create_time IS NULL, path, ?4),"
        "    cmdline = ?5, parent_path = excluded.parent_path"
        "  WHERE create_time IS NOT ?3;";

const char *const sqlUpsertConnProc = "INSERT INTO proc(pid, app_id, path) VALUES(?1, ?2, ?3)"
                                      "  ON CONFLICT(pid) DO UPDATE SET app_id = ?2, path = ?3"
                                      "  WHERE app_id IS NOT ?2 OR path IS NOT ?3;";

const char *const sqlUpdateProcPath =
        "UPDATE proc SET path = ?2 WHERE pid = ?1 AND path IS NOT ?2;";

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

const char *const sqlUpdateFlowHost =
        "UPDATE flow SET remote_host = ?1"
        "  WHERE pid = ?2 AND remote_ip IS ?3 AND remote_ip6 IS ?4 AND remote_host IS NULL;";

const char *const sqlSelectFlowHosts =
        "SELECT DISTINCT pid, remote_ip, remote_ip6, remote_host FROM flow"
        "  WHERE remote_host IS NOT NULL;";

const char *const sqlDeleteFlow = "DELETE FROM flow WHERE flow_id = ?1;";

const char *const sqlDeleteProcFlows = "DELETE FROM flow WHERE pid = ?1;";

const char *const sqlDeleteProc = "DELETE FROM proc WHERE pid = ?1;";

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
        NetDbUtil::ip4Var(entry.localIp(), isIPv6),
        NetDbUtil::ip4Var(entry.remoteIp(), isIPv6),
        NetDbUtil::ip6Var(entry.localIp(), isIPv6),
        NetDbUtil::ip6Var(entry.remoteIp(), isIPv6),
        DbVar::nullable(entry.remoteHost()),
        DbVar::nullable(entry.inheritPath()),
        entry.connTime(),
    };
}

// The columns: pid, remote_ip, remote_ip6
ProcessIpKey flowHostKey(const SqliteStmt &stmt)
{
    const bool isIPv6 = NetDbUtil::columnIsIPv6(stmt, 1);

    return ProcessIpKey(stmt.columnUInt(0), NetDbUtil::columnIp(stmt, 1, 2, isIPv6), isIPv6);
}

// The services' command lines only: the others may hold secrets, the DB is readable by all
QString serviceCommandLine(const LogEntryProcStart &entry, const QString &path)
{
    if (!FileUtil::isSvcHostExe(path))
        return {};

    const ProcessInfo pi(entry.pid());

    // The exited process's id may be reused
    if (pi.createTime() != entry.createTime())
        return {};

    return pi.commandLine();
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
        restoreFlowHosts();

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

// The host names of the previous run's flows (kept on its stop), for their connections logged
// again; not of a run before the boot, as the processes' ids are reused
void ActivityManager::restoreFlowHosts()
{
    const QString filePath = sqliteDb()->filePath();
    if (!FileUtil::fileExists(filePath) || FileUtil::fileModTime(filePath) < OsUtil::bootTime())
        return;

    SqliteDb db(filePath, SqliteDb::OpenDefaultReadOnly);
    if (!db.open())
        return;

    SqliteStmt stmt;
    if (!DbQuery(&db).sql(sqlSelectFlowHosts).prepare(stmt))
        return;

    auto dnsClientManager = Fort::dependency<DnsClientManager>();

    while (stmt.step() == SqliteStmt::StepRow) {
        dnsClientManager->addFlowHost(flowHostKey(stmt), stmt.columnText(3));
    }
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

void ActivityManager::logConn(const LogEntryConn &entry)
{
    if (!beginChanges())
        return;

    // Logged again (after a blocked reauthorization): the flow is kept, its reason may be changed
    if (insertFlow(entry)) {
        waitConnHost(entry);
    } else {
        updateFlowReason(entry);
    }

    // The process's Program may be changed; a System's process isn't logged by PROC_START
    upsertConnProc(entry);
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

void ActivityManager::upsertConnProc(const LogEntryConn &entry)
{
    doneChange(sqlUpsertConnProc, { entry.pid(), DbVar::nullable(entry.appId()), entry.path() });
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

void ActivityManager::logProcNew(const LogEntryProcNew &entry, const QString &path)
{
    if (!beginChanges())
        return;

    // The process of a known flow is logged again: its name is changed
    doneChange(sqlUpdateProcPath, { entry.pid(), path });
}

void ActivityManager::logProcStart(const LogEntryProcStart &entry)
{
    if (!beginChanges())
        return;

    const QString path = entry.path();

    doneChange(sqlUpsertProcStart,
            { entry.pid(), DbVar::nullable(entry.parentPid()), DbVar::nullable(entry.createTime()),
                    path, DbVar::nullable(serviceCommandLine(entry, path)) });
}

void ActivityManager::logProcExit(const LogEntryProcExit &entry)
{
    // The process's flows are closed after its exit: removed with it
    deleteById(sqlDeleteProcFlows, entry.pid());
    deleteById(sqlDeleteProc, entry.pid());
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
    deleteById(sqlDeleteFlow, entry.flowId());
}

void ActivityManager::logStatTraf(const LogEntryStatTraf &entry)
{
    const int procCount = entry.procCount();

    for (int i = 0; i < procCount; ++i) {
        const LogEntryStatTraf::ProcTraf procTraf = entry.procTraf(i);

        // The process's flows are closed: their FLOW_CLOSED may be lost by the log's overflow
        if (procTraf.inactive) {
            deleteById(sqlDeleteProcFlows, procTraf.pid);
        }
    }
}

void ActivityManager::deleteById(const char *sql, quint64 id)
{
    if (!beginChanges())
        return;

    doneChange(sql, { qint64(id) });
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

    doneChange(sqlUpdateFlowHost,
            { name, key.pid, NetDbUtil::ip4Var(key.ip, isIPv6),
                    NetDbUtil::ip6Var(key.ip, isIPv6) });
}

void ActivityManager::logTime()
{
    ++m_tick;
}

void ActivityManager::commitChanges()
{
    if (!m_inChanges)
        return;

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
    if (!beginChanges())
        return;

    DbQuery(sqliteDb()).sql("DELETE FROM flow;").executeOk();
    DbQuery(sqliteDb()).sql("DELETE FROM proc;").executeOk();

    m_changed = true;

    commitChanges();
}
