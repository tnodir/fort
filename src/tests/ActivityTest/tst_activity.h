#pragma once

#include <QCoreApplication>

#include <googletest.h>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitestmt.h>

#include <activity/activitymanager.h>
#include <common/fortlog.h>
#include <driver/drivercommon.h>
#include <log/logentryconn.h>
#include <log/logentryflowclosed.h>
#include <log/logentryflowtraf.h>
#include <log/logentryprocexit.h>
#include <log/logentryprocnew.h>
#include <log/logentryprocstart.h>
#include <log/logentrystattraf.h>
#include <util/net/processipkey.h>
#include <util/processinfo.h>

namespace {

inline constexpr quint32 testPid = 100;
inline constexpr quint32 testParentPid = 10;
inline constexpr qint64 testCreateTime = 12345;
inline constexpr quint32 testAppId = 7;
inline constexpr char testPath[] = "C:\\test\\app.exe";
inline constexpr char testParentPath[] = "C:\\test\\parent.exe";

LogEntryConn makeConn(quint64 flowId, quint32 pid = testPid)
{
    LogEntryConn entry;
    entry.setPid(pid);
    entry.setKernelPath(testPath);
    entry.setBlocked(false);
    entry.setFlowLogging(true);
    entry.setInbound(true);
    entry.setIpProto(6);
    entry.setLocalPort(1000);
    entry.setRemotePort(443);
    entry.setLocalIp4(0x0A000001);
    entry.setRemoteIp4(0x08080808);
    entry.setConnTime(1000);
    entry.setFlowId(flowId);
    entry.setRemoteHost("example.com");
    return entry;
}

LogEntryProcNew makeProcNew(quint32 pid = testPid)
{
    LogEntryProcNew entry;
    entry.setPid(pid);
    entry.setParentPid(testParentPid);
    entry.setCreateTime(testCreateTime);
    return entry;
}

LogEntryProcStart makeProcStart(
        quint32 pid, quint32 parentPid, qint64 createTime, const QString &kernelPath = testPath)
{
    LogEntryProcStart entry(pid, kernelPath);
    entry.setParentPid(parentPid);
    entry.setCreateTime(createTime);
    return entry;
}

// The process and its parent are started
void logProcStarts(ActivityManager &manager)
{
    manager.logProcStart(makeProcStart(testParentPid, 1, testCreateTime - 1, testParentPath));
    manager.logProcStart(makeProcStart(testPid, testParentPid, testCreateTime));
}

QByteArray flowTrafData(const QVector<FORT_FLOW_STAT> &flowStats)
{
    QByteArray data(flowStats.size() * DriverCommon::logFlowStatSize(), Qt::Uninitialized);

    char *out = data.data();
    for (const FORT_FLOW_STAT &flowStat : flowStats) {
        out = DriverCommon::logFlowStatWrite(out, flowStat);
    }

    return data;
}

void logFlowTraf(ActivityManager &manager, const QVector<FORT_FLOW_STAT> &flowStats)
{
    const QByteArray data = flowTrafData(flowStats);

    manager.logFlowTraf(LogEntryFlowTraf(flowStats.size(), data.constData()));
}

void logFlowClosed(ActivityManager &manager, quint64 flowId)
{
    LogEntryFlowClosed entry(flowId);

    manager.logFlowClosed(entry);
}

void logProcInactive(ActivityManager &manager, quint32 pid)
{
    const quint32 procTrafBytes[] = { pid | 1, 0, 0 };

    manager.logStatTraf(LogEntryStatTraf(1, procTrafBytes));
}

int g_warningCount = 0;

void countWarnings(QtMsgType type, const QMessageLogContext & /*context*/, const QString & /*msg*/)
{
    if (type == QtWarningMsg) {
        ++g_warningCount;
    }
}

}

class ActivityTest : public Test
{
    // Test interface
protected:
    void SetUp();

    enum FlowColumn {
        FlowPid = 0,
        FlowInbound,
        FlowLocalNet,
        FlowIpProto,
        FlowLocalPort,
        FlowRemotePort,
        FlowLocalIp,
        FlowRemoteIp,
        FlowLocalIp6,
        FlowRemoteIp6,
        FlowRemoteHost,
        FlowConnTime,
        FlowInBytes,
        FlowOutBytes,
        FlowInSpeed,
        FlowOutSpeed,
        FlowColumnCount,
    };

    QVariantList flowRow(quint64 flowId) const;
    QVariantList procRow(quint32 pid) const;
    QVariant procCmdLine(quint32 pid) const;

    QByteArray flowIp6(quint64 flowId, const char *column) const;

    int flowCount() const { return count("flow"); }
    int procCount() const { return count("proc"); }

    qint64 inSpeed(quint64 flowId) const { return flowRow(flowId).value(FlowInSpeed).toLongLong(); }

    // The driver's log buffer is processed
    void endBuffer() { m_manager.commitChanges(); }

    // The tick's last FLOW_TRAF portion without the flows' bytes
    void endTick() { logFlowTraf(m_manager, {}); }

    // The log buffer of the driver's tick without traffic
    void tickBuffer()
    {
        m_manager.logTime();
        endTick();
        endBuffer();
    }

protected:
    ActivityManager m_manager { ":memory:" };
    int m_changedCount = 0;

private:
    int count(const char *table) const;
};

void ActivityTest::SetUp()
{
    Q_INIT_RESOURCE(activity_migrations);

    m_manager.setUp();

    QObject::connect(&m_manager, &ActivityManager::activityChanged, [&] { ++m_changedCount; });
}

int ActivityTest::count(const char *table) const
{
    return DbQuery(m_manager.sqliteDb())
            .sql(QString("SELECT COUNT(*) FROM %1;").arg(table))
            .execute()
            .toInt();
}

QVariantList ActivityTest::flowRow(quint64 flowId) const
{
    return DbQuery(m_manager.sqliteDb())
            .sql("SELECT pid, inbound, local_net, ip_proto, local_port, remote_port, local_ip, "
                 "remote_ip,"
                 "    local_ip6 IS NOT NULL, remote_ip6 IS NOT NULL, remote_host, conn_time, "
                 "in_bytes, out_bytes,"
                 "    in_speed, out_speed"
                 "  FROM flow WHERE flow_id = ?1;")
            .vars({ qint64(flowId) })
            .execute(FlowColumnCount)
            .toList();
}

QByteArray ActivityTest::flowIp6(quint64 flowId, const char *column) const
{
    SqliteStmt stmt;

    if (!DbQuery(m_manager.sqliteDb())
                    .sql(QString("SELECT %1 FROM flow WHERE flow_id = ?1;").arg(column))
                    .vars({ qint64(flowId) })
                    .prepareRow(stmt))
        return {};

    return stmt.columnBlob(0);
}

QVariantList ActivityTest::procRow(quint32 pid) const
{
    return DbQuery(m_manager.sqliteDb())
            .sql("SELECT app_id, parent_pid, path, parent_path FROM proc WHERE pid = ?1;")
            .vars({ pid })
            .execute(4)
            .toList();
}

QVariant ActivityTest::procCmdLine(quint32 pid) const
{
    return DbQuery(m_manager.sqliteDb())
            .sql("SELECT cmdline FROM proc WHERE pid = ?1;")
            .vars({ pid })
            .execute();
}

TEST_F(ActivityTest, connAddsFlowAndProcess)
{
    LogEntryConn conn = makeConn(1);
    conn.setAppId(testAppId);
    conn.setLocalNet(true);

    m_manager.logConn(conn);

    // Not committed till the log buffer's end
    ASSERT_EQ(m_changedCount, 0);

    endBuffer();
    ASSERT_EQ(m_changedCount, 1);

    const QVariantList flow = flowRow(1);
    ASSERT_EQ(flow.size(), FlowColumnCount);
    ASSERT_EQ(flow[FlowPid].toUInt(), testPid);
    ASSERT_TRUE(flow[FlowInbound].toBool());
    ASSERT_TRUE(flow[FlowLocalNet].toBool());
    ASSERT_EQ(flow[FlowIpProto].toInt(), 6);
    ASSERT_EQ(flow[FlowLocalPort].toInt(), 1000);
    ASSERT_EQ(flow[FlowRemotePort].toInt(), 443);
    ASSERT_EQ(quint32(flow[FlowLocalIp].toInt()), 0x0A000001u);
    ASSERT_EQ(quint32(flow[FlowRemoteIp].toInt()), 0x08080808u);
    ASSERT_FALSE(flow[FlowLocalIp6].toBool());
    ASSERT_EQ(flow[FlowRemoteHost].toString(), "example.com");
    ASSERT_EQ(flow[FlowConnTime].toLongLong(), 1000);
    ASSERT_EQ(flow[FlowInBytes].toLongLong(), 0);
    ASSERT_EQ(flow[FlowInSpeed].toLongLong(), 0);

    // The process without PROC_START, a System's one: its parent is unknown
    const QVariantList process = procRow(testPid);
    ASSERT_EQ(process.size(), 4);
    ASSERT_EQ(process[0].toUInt(), testAppId);
    ASSERT_TRUE(process[1].isNull());
    ASSERT_EQ(process[2].toString(), conn.path());
    ASSERT_TRUE(process[3].isNull());
}

TEST_F(ActivityTest, procStartAddsProcess)
{
    logProcStarts(m_manager);
    endBuffer();
    ASSERT_EQ(m_changedCount, 1);

    // The processes without flows are kept
    ASSERT_EQ(procCount(), 2);

    // The parent's path is of its row
    QVariantList process = procRow(testPid);
    ASSERT_TRUE(process[0].isNull());
    ASSERT_EQ(process[1].toUInt(), testParentPid);
    ASSERT_EQ(process[2].toString(), makeProcStart(testPid, 0, 0).path());
    ASSERT_EQ(process[3].toString(), makeProcStart(testParentPid, 0, 0, testParentPath).path());

    // The flow's CONN sets the Program
    LogEntryConn conn = makeConn(1);
    conn.setAppId(testAppId);
    m_manager.logConn(conn);
    endBuffer();

    process = procRow(testPid);
    ASSERT_EQ(process[0].toUInt(), testAppId);
    ASSERT_EQ(process[1].toUInt(), testParentPid);
    ASSERT_FALSE(process[3].isNull());
}

TEST_F(ActivityTest, procStartParentCreatedLater)
{
    // The parent's id is reused by a newer process
    m_manager.logProcStart(makeProcStart(testParentPid, 1, testCreateTime + 1, testParentPath));
    m_manager.logProcStart(makeProcStart(testPid, testParentPid, testCreateTime));
    endBuffer();

    ASSERT_TRUE(procRow(testPid)[3].isNull());

    // An unknown creation time is trusted
    m_manager.logProcStart(makeProcStart(testPid + 4, testParentPid, 0));
    endBuffer();

    ASSERT_FALSE(procRow(testPid + 4)[3].isNull());
}

TEST_F(ActivityTest, procStartLoggedAgainKept)
{
    logProcStarts(m_manager);

    LogEntryConn conn = makeConn(1);
    conn.setAppId(testAppId);
    m_manager.logConn(conn);
    endBuffer();
    const int changedCount = m_changedCount;

    // The process created during the enumeration is logged twice
    m_manager.logProcStart(makeProcStart(testPid, testParentPid, testCreateTime));
    endBuffer();

    ASSERT_EQ(procRow(testPid)[0].toUInt(), testAppId);
    ASSERT_EQ(m_changedCount, changedCount);
}

TEST_F(ActivityTest, procStartAfterConn)
{
    // The CONN is logged before its process's PROC_START: the conf is set before the enumeration
    LogEntryConn conn = makeConn(1);
    conn.setAppId(testAppId);
    m_manager.logConn(conn);
    endBuffer();

    m_manager.logProcStart(makeProcStart(testParentPid, 1, testCreateTime - 1, testParentPath));
    m_manager.logProcStart(
            makeProcStart(testPid, testParentPid, testCreateTime, "C:\\test\\exe.exe"));
    endBuffer();

    // The row keeps its Program and path, gets the process's tree
    const QVariantList process = procRow(testPid);
    ASSERT_EQ(process[0].toUInt(), testAppId);
    ASSERT_EQ(process[1].toUInt(), testParentPid);
    ASSERT_EQ(process[2].toString(), conn.path());
    ASSERT_FALSE(process[3].isNull());
}

TEST_F(ActivityTest, procStartReusedPid)
{
    logProcStarts(m_manager);

    LogEntryConn conn = makeConn(1);
    conn.setAppId(testAppId);
    m_manager.logConn(conn);
    endBuffer();

    // The exited process's PROC_EXIT is lost: its id is reused by a new process
    const LogEntryProcStart procStart =
            makeProcStart(testPid, testParentPid + 4, testCreateTime + 1, "C:\\test\\new.exe");
    m_manager.logProcStart(procStart);
    endBuffer();

    const QVariantList process = procRow(testPid);
    ASSERT_TRUE(process[0].isNull());
    ASSERT_EQ(process[1].toUInt(), testParentPid + 4);
    ASSERT_EQ(process[2].toString(), procStart.path());
    ASSERT_TRUE(process[3].isNull());
}

TEST_F(ActivityTest, procStartServiceCommandLine)
{
    // The current process is taken for a service's one
    const quint32 pid = ProcessInfo::currentPid();
    const qint64 createTime = ProcessInfo(pid).createTime();
    const QString svcHostPath = qEnvironmentVariable("SystemRoot") + R"(\System32\svchost.exe)";

    // The process's id is reused
    m_manager.logProcStart(makeProcStart(pid, 0, createTime + 1, svcHostPath));
    endBuffer();

    ASSERT_TRUE(procCmdLine(pid).isNull());

    m_manager.logProcStart(makeProcStart(pid, 0, createTime, svcHostPath));
    endBuffer();

    const QString cmdLine = ProcessInfo(pid).commandLine();
    ASSERT_FALSE(cmdLine.isEmpty());
    ASSERT_EQ(procCmdLine(pid).toString(), cmdLine);

    // The other processes' command lines aren't stored
    m_manager.logProcStart(makeProcStart(testPid, 0, 0, QCoreApplication::applicationFilePath()));
    endBuffer();

    ASSERT_TRUE(procCmdLine(testPid).isNull());

    ASSERT_EQ(m_manager.serviceCommandLines(), (QHash<quint32, QString> { { pid, cmdLine } }));
}

TEST_F(ActivityTest, procExitRemovesProcessAndFlows)
{
    logProcStarts(m_manager);
    m_manager.logConn(makeConn(1));
    m_manager.logConn(makeConn(2, testParentPid));
    endBuffer();

    m_manager.logProcExit(LogEntryProcExit(testPid));
    endBuffer();

    ASSERT_TRUE(procRow(testPid).isEmpty());
    ASSERT_TRUE(flowRow(1).isEmpty());
    ASSERT_FALSE(procRow(testParentPid).isEmpty());
    ASSERT_FALSE(flowRow(2).isEmpty());
}

TEST_F(ActivityTest, connIPv6)
{
    LogEntryConn conn = makeConn(1);
    conn.setIsIPv6(true);
    conn.localIp().v6.addr32[3] = 0x01020304;
    conn.remoteIp().v6.addr32[0] = 0x05060708;

    m_manager.logConn(conn);
    endBuffer();

    const QVariantList flow = flowRow(1);
    ASSERT_TRUE(flow[FlowLocalIp].isNull());
    ASSERT_TRUE(flow[FlowRemoteIp].isNull());
    ASSERT_TRUE(flow[FlowLocalIp6].toBool());
    ASSERT_TRUE(flow[FlowRemoteIp6].toBool());
    ASSERT_EQ(flowIp6(1, "local_ip6"), conn.localIp6View().toByteArray());
    ASSERT_EQ(flowIp6(1, "remote_ip6"), conn.remoteIp6View().toByteArray());

    // The process's Program and parent are unknown
    const QVariantList process = procRow(testPid);
    ASSERT_TRUE(process[0].isNull());
    ASSERT_TRUE(process[1].isNull());
    ASSERT_TRUE(process[3].isNull());
}

TEST_F(ActivityTest, flowTrafSumsBytesAndSpeeds)
{
    m_manager.logConn(makeConn(1));
    endBuffer();
    const int changedCount = m_changedCount;

    m_manager.logTime();
    logFlowTraf(m_manager, { { .flow_id = 1, .traf = { .in_bytes = 100, .out_bytes = 10 } } });
    endBuffer();

    // The tick's bytes are committed by their log buffer, not by the next tick's TIME
    ASSERT_EQ(m_changedCount, changedCount + 1);

    QVariantList flow = flowRow(1);
    ASSERT_EQ(flow[FlowInBytes].toLongLong(), 100);
    ASSERT_EQ(flow[FlowOutBytes].toLongLong(), 10);
    ASSERT_EQ(flow[FlowInSpeed].toLongLong(), 200); // 100 bytes per 500 msecs
    ASSERT_EQ(flow[FlowOutSpeed].toLongLong(), 20);

    m_manager.logTime();
    logFlowTraf(m_manager, { { .flow_id = 1, .traf = { .in_bytes = 50, .out_bytes = 5 } } });
    endBuffer();

    flow = flowRow(1);
    ASSERT_EQ(flow[FlowInBytes].toLongLong(), 150);
    ASSERT_EQ(flow[FlowOutBytes].toLongLong(), 15);
    ASSERT_EQ(flow[FlowInSpeed].toLongLong(), 100);
    ASSERT_EQ(flow[FlowOutSpeed].toLongLong(), 10);
}

TEST_F(ActivityTest, idleFlowHasNoSpeed)
{
    m_manager.logConn(makeConn(1));
    m_manager.logTime();
    logFlowTraf(m_manager, { { .flow_id = 1, .traf = { .in_bytes = 100 } } });
    endBuffer();
    ASSERT_EQ(inSpeed(1), 200);

    // The next tick's portions may be in its later log buffer: the speed is kept till its end
    m_manager.logTime();
    endBuffer();
    ASSERT_EQ(inSpeed(1), 200);

    endTick();
    endBuffer();
    ASSERT_EQ(inSpeed(1), 0);
    ASSERT_EQ(flowRow(1)[FlowInBytes].toLongLong(), 100);

    // No changes, no signal
    const int changedCount = m_changedCount;
    tickBuffer();
    ASSERT_EQ(m_changedCount, changedCount);

    // No speeds to clear: no write transaction
    m_manager.logTime();
    endTick();
    ASSERT_TRUE(m_manager.sqliteDb()->isAutoCommit());
    endBuffer();

    // The new bytes' speed is cleared again
    m_manager.logTime();
    logFlowTraf(m_manager, { { .flow_id = 1, .traf = { .in_bytes = 100 } } });
    endBuffer();
    tickBuffer();
    ASSERT_EQ(inSpeed(1), 0);
}

TEST_F(ActivityTest, fullFlowTrafPortionContinuesTick)
{
    m_manager.logConn(makeConn(1));
    m_manager.logConn(makeConn(2));
    m_manager.logTime();
    logFlowTraf(m_manager,
            { { .flow_id = 1, .traf = { .in_bytes = 100 } },
                    { .flow_id = 2, .traf = { .in_bytes = 100 } } });
    endBuffer();

    // A full portion of the other flows: the tick goes on
    QVector<FORT_FLOW_STAT> otherFlows(FORT_LOG_FLOW_TRAF_BUFFER_FLOW_COUNT);
    for (int i = 0; i < otherFlows.size(); ++i) {
        otherFlows[i] = { .flow_id = quint64(1000 + i), .traf = { .in_bytes = 1 } };
    }

    m_manager.logTime();
    logFlowTraf(m_manager, otherFlows);
    endBuffer();
    ASSERT_EQ(inSpeed(1), 200);
    ASSERT_EQ(inSpeed(2), 200);

    // The shorter portion ends the tick
    logFlowTraf(m_manager, { { .flow_id = 2, .traf = { .in_bytes = 50 } } });
    endBuffer();
    ASSERT_EQ(inSpeed(1), 0);
    ASSERT_EQ(inSpeed(2), 100);
}

TEST_F(ActivityTest, unknownFlowBytesIgnored)
{
    m_manager.logConn(makeConn(1));
    endBuffer();
    const int changedCount = m_changedCount;

    m_manager.logTime();
    logFlowTraf(m_manager, { { .flow_id = 2, .traf = { .in_bytes = 100 } } });
    endBuffer();

    ASSERT_EQ(flowCount(), 1);
    ASSERT_EQ(flowRow(1)[FlowInBytes].toLongLong(), 0);
    ASSERT_EQ(m_changedCount, changedCount);
}

TEST_F(ActivityTest, reloggedConnKeepsFlow)
{
    m_manager.logConn(makeConn(1));
    m_manager.logTime();
    logFlowTraf(m_manager, { { .flow_id = 1, .traf = { .in_bytes = 100 } } });
    endBuffer();
    tickBuffer(); // the speed is cleared
    const int changedCount = m_changedCount;

    // An allowed reauthorization after a blocked one logs the flow's CONN again
    m_manager.logConn(makeConn(1));
    endBuffer();

    ASSERT_EQ(flowCount(), 1);
    ASSERT_EQ(flowRow(1)[FlowInBytes].toLongLong(), 100);
    ASSERT_EQ(m_changedCount, changedCount);
}

TEST_F(ActivityTest, reloggedConnChangesProgram)
{
    logProcStarts(m_manager);
    m_manager.logConn(makeConn(1));
    endBuffer();
    const int changedCount = m_changedCount;

    // An allowed reauthorization after a blocked one logs the flow's CONN with its new Program
    LogEntryConn conn = makeConn(1);
    conn.setAppId(testAppId);

    m_manager.logConn(conn);
    endBuffer();

    const QVariantList process = procRow(testPid);
    ASSERT_EQ(process[0].toUInt(), testAppId);
    ASSERT_EQ(process[1].toUInt(), testParentPid);
    ASSERT_FALSE(process[3].isNull());
    ASSERT_EQ(m_changedCount, changedCount + 1);

    // The Program is deleted
    m_manager.logConn(makeConn(1));
    endBuffer();

    ASSERT_TRUE(procRow(testPid)[0].isNull());
    ASSERT_EQ(m_changedCount, changedCount + 2);
}

TEST_F(ActivityTest, procNewUpdatesProcess)
{
    // The CONN is logged with the paths before its process's PROC_NEW
    logProcStarts(m_manager);
    m_manager.logConn(makeConn(1));
    endBuffer();
    const int changedCount = m_changedCount;

    m_manager.logProcNew(makeProcNew(), "C:\\test\\service.exe");
    endBuffer();

    // Only the name is changed
    const QVariantList process = procRow(testPid);
    ASSERT_EQ(process[1].toUInt(), testParentPid);
    ASSERT_EQ(process[2].toString(), "C:\\test\\service.exe");
    ASSERT_FALSE(process[3].isNull());
    ASSERT_EQ(m_changedCount, changedCount + 1);

    // The process without PROC_START isn't added
    m_manager.logProcNew(makeProcNew(testPid + 4), "C:\\test\\other.exe");
    endBuffer();

    ASSERT_EQ(procCount(), 2);
    ASSERT_EQ(m_changedCount, changedCount + 1);
}

TEST_F(ActivityTest, procNewSamePathUnchanged)
{
    // The CONN is logged with the paths before its process's PROC_NEW
    m_manager.logConn(makeConn(1));
    endBuffer();
    const int changedCount = m_changedCount;

    m_manager.logProcNew(makeProcNew(), procRow(testPid)[2].toString());
    endBuffer();

    ASSERT_EQ(m_changedCount, changedCount);
}

TEST_F(ActivityTest, flowClosedKeepsProcess)
{
    m_manager.logConn(makeConn(1));
    m_manager.logConn(makeConn(2));
    endBuffer();

    logFlowClosed(m_manager, 1);
    endBuffer();

    ASSERT_TRUE(flowRow(1).isEmpty());
    ASSERT_FALSE(flowRow(2).isEmpty());
    ASSERT_EQ(procCount(), 1);

    logFlowClosed(m_manager, 2);
    endBuffer();

    ASSERT_EQ(flowCount(), 0);
    ASSERT_EQ(procCount(), 1);
}

TEST_F(ActivityTest, closedProcessRemovesFlows)
{
    m_manager.logConn(makeConn(1));
    m_manager.logConn(makeConn(2, testPid + 4));
    endBuffer();

    // The FLOW_CLOSED are lost by the log's overflow
    logProcInactive(m_manager, testPid);
    endBuffer();

    ASSERT_TRUE(flowRow(1).isEmpty());
    ASSERT_FALSE(flowRow(2).isEmpty());
    ASSERT_EQ(procCount(), 2);
}

TEST_F(ActivityTest, clearRemovesAll)
{
    m_manager.logConn(makeConn(1));
    endBuffer();

    m_manager.logConn(makeConn(2));
    m_manager.clear();

    ASSERT_EQ(flowCount(), 0);
    ASSERT_EQ(procCount(), 0);
    ASSERT_EQ(m_changedCount, 2);
}

TEST_F(ActivityTest, lateHostNameUpdatesFlowWithoutHost)
{
    LogEntryConn conn = makeConn(1);
    conn.setRemoteHost({});
    m_manager.logConn(conn);
    m_manager.logConn(makeConn(2, testPid + 4));
    endBuffer();

    ASSERT_TRUE(flowRow(1).value(FlowRemoteHost).isNull());

    ip_addr_t remoteIp = {};
    remoteIp.v4 = 0x08080808;

    const int changedCount = m_changedCount;

    // Committed at once, out of a log buffer's processing
    m_manager.updateConnHost(ProcessIpKey(testPid, remoteIp, /*isRemoteIPv6=*/false), "dns.google");

    ASSERT_TRUE(m_manager.sqliteDb()->isAutoCommit());
    ASSERT_EQ(flowRow(1).value(FlowRemoteHost).toString(), "dns.google");
    ASSERT_EQ(m_changedCount, changedCount + 1);

    // The flow with the host name is kept, as the other process's one
    m_manager.updateConnHost(ProcessIpKey(testPid, remoteIp, false), "other.name");
    m_manager.updateConnHost(ProcessIpKey(testPid + 4, remoteIp, false), "other.name");

    ASSERT_TRUE(m_manager.sqliteDb()->isAutoCommit());
    ASSERT_EQ(flowRow(1).value(FlowRemoteHost).toString(), "dns.google");
    ASSERT_EQ(flowRow(2).value(FlowRemoteHost).toString(), "example.com");
    ASSERT_EQ(m_changedCount, changedCount + 1);
}

TEST_F(ActivityTest, failedDbLogsNoEntryErrors)
{
    // The DB's file can't be created under a file
    ActivityManager manager(QCoreApplication::applicationFilePath() + "/activity");
    manager.setUp();

    g_warningCount = 0;
    const QtMessageHandler oldHandler = qInstallMessageHandler(countWarnings);

    manager.logConn(makeConn(1));
    logFlowTraf(manager, { { .flow_id = 1, .traf = { .in_bytes = 100 } } });
    manager.commitChanges();

    qInstallMessageHandler(oldHandler);

    ASSERT_EQ(g_warningCount, 0);
}
