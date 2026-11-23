#include "activitysnapshot.h"

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <activity/activitymanager.h>
#include <fortglobal.h>
#include <util/net/netutil.h>

using namespace Fort;

namespace {

inline constexpr const char *sqlSelectProcs =
        "SELECT pid, app_id, parent_pid, path, parent_path FROM proc;";

inline constexpr const char *sqlSelectFlows =
        "SELECT flow_id, pid, inbound, reason, ip_proto, local_port, remote_port,"
        "    local_ip, remote_ip, local_ip6, remote_ip6, remote_host, conn_time,"
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

// The columns: local_ip, remote_ip, local_ip6, remote_ip6
void readFlowIps(const SqliteStmt &stmt, ActivityFlowRow &row)
{
    const QByteArray localIp6 = stmt.columnBlob(9);
    if (localIp6.size() != int(sizeof(ip6_addr_t))) {
        row.localIp.v4 = quint32(stmt.columnInt(7));
        row.remoteIp.v4 = quint32(stmt.columnInt(8));
        return;
    }

    row.isIPv6 = true;
    row.localIp.v6 = NetUtil::arrayViewToIp6(localIp6);
    row.remoteIp.v6 = NetUtil::arrayViewToIp6(stmt.columnBlob(10));
}

ActivityFlowRow readFlowRow(const SqliteStmt &stmt)
{
    ActivityFlowRow row;
    row.flowId = stmt.columnUInt64(0);
    row.pid = stmt.columnUInt(1);
    row.inbound = stmt.columnBool(2);
    row.reason = quint8(stmt.columnInt(3));
    row.ipProto = quint8(stmt.columnInt(4));
    row.localPort = quint16(stmt.columnInt(5));
    row.remotePort = quint16(stmt.columnInt(6));

    readFlowIps(stmt, row);

    row.remoteHost = stmt.columnText(11);
    row.connTime = stmt.columnUnixTime(12);
    row.traf.inBytes = stmt.columnInt64(13);
    row.traf.outBytes = stmt.columnInt64(14);
    row.traf.inSpeed = stmt.columnInt64(15);
    row.traf.outSpeed = stmt.columnInt64(16);
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
