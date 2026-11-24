#include "activitysnapshot.h"

#include <QSet>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <activity/activitymanager.h>
#include <fortglobal.h>
#include <model/connsearchmodel.h>
#include <util/net/netdbutil.h>

using namespace Fort;

namespace {

// Only the processes with flows are shown
inline constexpr const char *sqlSelectProcs =
        "SELECT pid, app_id, parent_pid, path, parent_path FROM proc"
        "  WHERE EXISTS (SELECT 1 FROM flow WHERE flow.pid = proc.pid);";

inline constexpr const char *sqlSelectFlows =
        "SELECT flow_id, pid, terminated, inherited, inbound, loopback, reason, ip_proto,"
        "    zone_id, rule_id, local_port, remote_port, local_ip, remote_ip, local_ip6,"
        "    remote_ip6, remote_host, inherit_path, conn_time,"
        "    in_bytes, out_bytes, in_speed, out_speed"
        "  FROM flow;";

ActivityProcRow readProcRow(const SqliteStmt &stmt)
{
    ActivityProcRow row;
    row.pid = stmt.columnUInt(0);
    row.appId = stmt.columnInt64(1);
    row.parentPid = stmt.columnUInt(2);
    row.path = stmt.columnText(3);
    row.parentPath = stmt.columnText(4);
    return row;
}

ActivityFlowRow readFlowRow(const SqliteStmt &stmt)
{
    ActivityFlowRow row;
    row.flowId = stmt.columnUInt64(0);
    row.pid = stmt.columnUInt(1);
    row.terminated = stmt.columnBool(2);
    row.inherited = stmt.columnBool(3);
    row.inbound = stmt.columnBool(4);
    row.loopback = stmt.columnBool(5);
    row.reason = quint8(stmt.columnInt(6));
    row.ipProto = quint8(stmt.columnInt(7));
    row.zoneId = quint8(stmt.columnInt(8));
    row.ruleId = quint16(stmt.columnInt(9));
    row.localPort = quint16(stmt.columnInt(10));
    row.remotePort = quint16(stmt.columnInt(11));

    const bool isIPv6 = NetDbUtil::columnIsIPv6(stmt, 12);

    row.isIPv6 = isIPv6;
    row.localIp = NetDbUtil::columnIp(stmt, 12, 14, isIPv6);
    row.remoteIp = NetDbUtil::columnIp(stmt, 13, 15, isIPv6);

    row.remoteHost = stmt.columnText(16);
    row.inheritAppPath = stmt.columnText(17);
    row.connTime = stmt.columnUnixTime(18);
    row.traf.inBytes = stmt.columnInt64(19);
    row.traf.outBytes = stmt.columnInt64(20);
    row.traf.inSpeed = stmt.columnInt64(21);
    row.traf.outSpeed = stmt.columnInt64(22);
    return row;
}

template<class Fill>
bool readRows(SqliteDb *db, const char *sql, Fill fill)
{
    SqliteStmt stmt;
    if (!DbQuery(db).sql(sql).prepare(stmt))
        return false;

    SqliteStmt::StepResult res;
    while ((res = stmt.step()) == SqliteStmt::StepRow) {
        fill(stmt);
    }

    return res == SqliteStmt::StepDone;
}

ActivityAppKey appKeyOf(const ActivityProcRow &row)
{
    if (row.appId != 0) {
        return { .appId = row.appId };
    }

    return { .path = row.path };
}

ActivityShownProc &shownProcOf(ActivityShown &shown, const ActivitySnapshot &snapshot, quint32 pid)
{
    auto it = shown.procs.find(pid);
    if (it != shown.procs.end()) {
        return *it;
    }

    ActivityShownProc proc;
    proc.row = snapshot.procs.value(pid);
    proc.row.pid = pid;
    proc.appKey = appKeyOf(proc.row);

    return *shown.procs.insert(pid, proc);
}

}

bool ActivitySnapshot::read()
{
    SqliteDb *db = activityManager()->sqliteDb();

    // Both tables of one commit: a client reads while the master writes
    const bool inTransaction = db->beginTransaction();

    bool ok = readRows(db, sqlSelectProcs, [&](const SqliteStmt &stmt) {
        const auto row = readProcRow(stmt);
        procs.insert(row.pid, row);
    });

    ok = ok && readRows(db, sqlSelectFlows, [&](const SqliteStmt &stmt) {
        const auto row = readFlowRow(stmt);
        flows.insert(row.flowId, row);
    });

    if (inTransaction) {
        db->endTransaction(/*ok=*/false);
    }

    return ok;
}

void ActivitySnapshot::removeUnmatchedFlows(const TextMatcher &textMatcher)
{
    ConnSearchModel::AppTextHash appTexts;

    flows.removeIf([&](const auto &it) {
        Conn conn = it.value();
        conn.appPath = procs.value(conn.pid).path;

        return !ConnSearchModel::isConnMatched(textMatcher, conn, appTexts);
    });
}

ActivityShown ActivityShown::build(const ActivitySnapshot &snapshot)
{
    ActivityShown shown;

    for (const auto &flow : snapshot.flows) {
        auto &totals = shownProcOf(shown, snapshot, flow.pid).totals;

        ++totals.flowCount;
        totals.traf += flow.traf;
    }

    for (const auto &proc : std::as_const(shown.procs)) {
        shown.apps[proc.appKey] += proc.totals;
    }

    return shown;
}

void ActivityShown::removeProcsWithoutFlows(const ActivitySnapshot &snapshot)
{
    QSet<quint32> pids;
    for (const auto &flow : snapshot.flows) {
        pids.insert(flow.pid);
    }

    procs.removeIf([&](const auto &it) { return !pids.contains(it.key()); });

    QSet<ActivityAppKey> appKeys;
    for (const auto &proc : std::as_const(procs)) {
        appKeys.insert(proc.appKey);
    }

    apps.removeIf([&](const auto &it) { return !appKeys.contains(it.key()); });
}
