#include "logbuffer.h"

#include <driver/drivercommon.h>

#include "logentryapp.h"
#include "logentryconn.h"
#include "logentryflowclosed.h"
#include "logentryprockill.h"
#include "logentryprocnew.h"
#include "logentrystattraf.h"
#include "logentrytime.h"

namespace {

quint16 pathSize(const QString &path)
{
    return quint16(path.size()) * sizeof(wchar_t);
}

void writePath(char *output, const QString &path)
{
    path.toWCharArray((wchar_t *) output);
}

QString readPath(const char *input, quint16 pathLen)
{
    if (pathLen == 0)
        return {};

    return QString::fromWCharArray((const wchar_t *) input, pathLen / int(sizeof(wchar_t)));
}

}

LogBuffer::LogBuffer(int bufferSize, QObject *parent) :
    QObject(parent),
    m_array(bufferSize ? bufferSize : DriverCommon::bufferSize(), Qt::Initialization::Uninitialized)
{
}

void LogBuffer::reset(int top)
{
    m_top = top;
    m_offset = 0;
}

char *LogBuffer::output()
{
    return m_array.data() + m_top;
}

const char *LogBuffer::input() const
{
    return m_array.constData() + m_offset;
}

void LogBuffer::prepareFor(int len)
{
    const int newSize = m_top + len;

    if (newSize > m_array.size()) {
        m_array.resize(newSize);
    }
}

char *LogBuffer::appendEntry(quint32 entrySize)
{
    prepareFor(int(entrySize));

    char *output = this->output();

    m_top += int(entrySize);

    return output;
}

const char *LogBuffer::entryInput() const
{
    Q_ASSERT(m_offset < m_top);

    return input();
}

void LogBuffer::skipEntry(quint32 entrySize)
{
    m_offset += int(entrySize);
}

const char *LogBuffer::takeEntry(quint32 entrySize)
{
    const char *input = entryInput();

    skipEntry(entrySize);

    return input;
}

FortLogType LogBuffer::peekEntryType()
{
    if (m_offset >= m_top)
        return FORT_LOG_TYPE_NONE;

    const char *input = this->input();

    const auto type = DriverCommon::logType(input);

    return static_cast<FortLogType>(type);
}

void LogBuffer::writeEntryApp(const LogEntryApp *logEntry)
{
    const QString path = logEntry->kernelPath();
    const quint16 pathLen = pathSize(path);

    char *output = appendEntry(DriverCommon::logAppSize(pathLen));

    DriverCommon::logAppHeaderWrite(output, logEntry->blocked(), logEntry->pid(), pathLen);
    writePath(output + DriverCommon::logAppHeaderSize(), path);
}

void LogBuffer::readEntryApp(LogEntryApp *logEntry)
{
    const char *input = entryInput();

    const auto header = DriverCommon::logAppHeaderRead(input);

    logEntry->setBlocked(header.blocked);
    logEntry->setPid(header.pid);
    logEntry->setKernelPath(readPath(input + DriverCommon::logAppHeaderSize(), header.pathLen));

    skipEntry(DriverCommon::logAppSize(header.pathLen));
}

void LogBuffer::writeEntryConn(const LogEntryConn *logEntry)
{
    const QString path = logEntry->kernelPath();
    const quint16 pathLen = pathSize(path);

    const QString inheritPath = logEntry->inheritKernelPath();
    const quint16 inheritPathLen = pathSize(inheritPath);

    const bool isIPv6 = logEntry->isIPv6();

    char *output = appendEntry(DriverCommon::logConnSize(pathLen, inheritPathLen, isIPv6));

    const FORT_CONF_META_CONN conn = {
        .inbound = logEntry->inbound(),
        .isIPv6 = logEntry->isIPv6(),
        .is_loopback = logEntry->loopback(),
        .inherited = logEntry->inherited(),
        .app_asked = logEntry->appAsked(),
        .act = {
            .blocked = logEntry->blocked(),
            .drop_blocked = logEntry->dropped(),
            .conn_alert = logEntry->alerted(),
            .zone_id = logEntry->zoneId(),
        },
        .reason = logEntry->reason(),
        .ip_proto = logEntry->ipProto(),
        .rule_id = logEntry->ruleId(),
        .local_port = logEntry->localPort(),
        .remote_port = logEntry->remotePort(),
        .process_id = logEntry->pid(),
        .flow_id = logEntry->flowId(),
        .local_ip = logEntry->localIp(),
        .remote_ip = logEntry->remoteIp(),
        .app = {
            .data = {
                .app_id = logEntry->appId(),
            },
        },
    };

    DriverCommon::logConnHeaderWrite(output, &conn, pathLen, inheritPathLen);

    writePath(output + DriverCommon::logConnHeaderSize(isIPv6), path);
    writePath(output + DriverCommon::logConnInheritPathOffset(pathLen, isIPv6), inheritPath);
}

void LogBuffer::readEntryConn(LogEntryConn *logEntry)
{
    const char *input = entryInput();

    FORT_CONF_META_CONN conn;
    quint16 pathLen;
    quint16 inheritPathLen;

    DriverCommon::logConnHeaderRead(input, &conn, &pathLen, &inheritPathLen);

    const QString path = readPath(input + DriverCommon::logConnHeaderSize(conn.isIPv6), pathLen);
    const QString inheritPath = readPath(
            input + DriverCommon::logConnInheritPathOffset(pathLen, conn.isIPv6), inheritPathLen);

    logEntry->setBlocked(conn.act.blocked);
    logEntry->setDropped(conn.act.drop_blocked);
    logEntry->setAlerted(conn.act.conn_alert);
    logEntry->setIsIPv6(conn.isIPv6);
    logEntry->setInbound(conn.inbound);
    logEntry->setLoopback(conn.is_loopback);
    logEntry->setInherited(conn.inherited);
    logEntry->setAppAsked(conn.app_asked);
    logEntry->setInheritKernelPath(inheritPath);
    logEntry->setReason(conn.reason);
    logEntry->setIpProto(conn.ip_proto);
    logEntry->setZoneId(conn.act.zone_id);
    logEntry->setRuleId(conn.rule_id);
    logEntry->setLocalPort(conn.local_port);
    logEntry->setRemotePort(conn.remote_port);
    logEntry->setLocalIp(conn.local_ip);
    logEntry->setRemoteIp(conn.remote_ip);
    logEntry->setAppId(conn.app.data.app_id);
    logEntry->setPid(conn.process_id);
    logEntry->setFlowId(conn.flow_id);
    logEntry->setKernelPath(path);

    skipEntry(DriverCommon::logConnSize(pathLen, inheritPathLen, conn.isIPv6));
}

void LogBuffer::writeEntryProcNew(const LogEntryProcNew *logEntry)
{
    const QString path = logEntry->kernelPath();
    const quint16 pathLen = pathSize(path);

    char *output = appendEntry(DriverCommon::logProcNewSize(pathLen));

    DriverCommon::logProcNewHeaderWrite(output, logEntry->appId(), logEntry->pid(), pathLen);
    writePath(output + DriverCommon::logProcNewHeaderSize(), path);
}

void LogBuffer::readEntryProcNew(LogEntryProcNew *logEntry)
{
    const char *input = entryInput();

    const auto header = DriverCommon::logProcNewHeaderRead(input);

    logEntry->setAppId(header.appId);
    logEntry->setPid(header.pid);
    logEntry->setKernelPath(readPath(input + DriverCommon::logProcNewHeaderSize(), header.pathLen));

    skipEntry(DriverCommon::logProcNewSize(header.pathLen));
}

void LogBuffer::readEntryStatTraf(LogEntryStatTraf *logEntry)
{
    const char *input = entryInput();

    quint16 procCount;
    DriverCommon::logStatTrafHeaderRead(input, &procCount);

    logEntry->setProcCount(procCount);

    if (procCount != 0) {
        input += DriverCommon::logStatHeaderSize();
        logEntry->setProcTrafBytes(reinterpret_cast<const quint32 *>(input));
    }

    skipEntry(DriverCommon::logStatSize(procCount));
}

void LogBuffer::writeEntryTime(const LogEntryTime *logEntry)
{
    char *output = appendEntry(DriverCommon::logTimeSize());

    DriverCommon::logTimeWrite(output, logEntry->unixTime(), logEntry->systemTimeChanged());
}

void LogBuffer::readEntryTime(LogEntryTime *logEntry)
{
    const char *input = takeEntry(DriverCommon::logTimeSize());

    qint64 unixTime;
    int systemTimeChanged;
    DriverCommon::logTimeRead(input, &unixTime, &systemTimeChanged);

    logEntry->setSystemTimeChanged(systemTimeChanged != 0);
    logEntry->setUnixTime(unixTime);
}

void LogBuffer::readEntryProcKill(LogEntryProcKill *logEntry)
{
    const char *input = takeEntry(DriverCommon::logProcKillSize());

    quint32 pid;
    DriverCommon::logProcKillRead(input, &pid);

    logEntry->setPid(pid);
}

void LogBuffer::writeEntryFlowClosed(const LogEntryFlowClosed *logEntry)
{
    char *output = appendEntry(DriverCommon::logFlowClosedSize());

    DriverCommon::logFlowClosedWrite(output, logEntry->flowId());
}

void LogBuffer::readEntryFlowClosed(LogEntryFlowClosed *logEntry)
{
    const char *input = takeEntry(DriverCommon::logFlowClosedSize());

    quint64 flowId;
    DriverCommon::logFlowClosedRead(input, &flowId);

    logEntry->setFlowId(flowId);
}
