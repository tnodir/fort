#include "logbuffer.h"

#include <driver/drivercommon.h>

#include "logentryapp.h"
#include "logentryconn.h"
#include "logentryprockill.h"
#include "logentryprocnew.h"
#include "logentrystattraf.h"
#include "logentrytime.h"

namespace {

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
    const quint16 pathLen = quint16(path.size()) * sizeof(wchar_t);

    const int entrySize = int(DriverCommon::logAppSize(pathLen));
    prepareFor(entrySize);

    char *output = this->output();

    DriverCommon::logAppHeaderWrite(output, logEntry->blocked(), logEntry->pid(), pathLen);

    if (pathLen > 0) {
        output += DriverCommon::logAppHeaderSize();
        path.toWCharArray((wchar_t *) output);
    }

    m_top += entrySize;
}

void LogBuffer::readEntryApp(LogEntryApp *logEntry)
{
    Q_ASSERT(m_offset < m_top);

    const char *input = this->input();

    int blocked;
    quint32 pid;
    quint16 pathLen;
    DriverCommon::logAppHeaderRead(input, &blocked, &pid, &pathLen);

    const QString path = readPath(input + DriverCommon::logAppHeaderSize(), pathLen);

    logEntry->setBlocked(blocked);
    logEntry->setPid(pid);
    logEntry->setKernelPath(path);

    const int entrySize = int(DriverCommon::logAppSize(pathLen));
    m_offset += entrySize;
}

void LogBuffer::writeEntryConn(const LogEntryConn *logEntry)
{
    const QString path = logEntry->kernelPath();
    const quint32 pathLen = quint32(path.size()) * sizeof(wchar_t);

    const QString inheritPath = logEntry->inheritKernelPath();
    const quint32 inheritPathLen = quint32(inheritPath.size()) * sizeof(wchar_t);

    const bool isIPv6 = logEntry->isIPv6();
    const int entrySize = int(DriverCommon::logConnSize(pathLen, inheritPathLen, isIPv6));
    prepareFor(entrySize);

    char *output = this->output();

    const FORT_CONF_META_CONN conn = {
        .inbound = logEntry->inbound(),
        .isIPv6 = logEntry->isIPv6(),
        .is_loopback = logEntry->loopback(),
        .inherited = logEntry->inherited(),
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
        .local_ip = logEntry->localIp(),
        .remote_ip = logEntry->remoteIp(),
        .app = {
            .data = {
                .app_id = logEntry->appId(),
            },
        },
    };

    DriverCommon::logConnHeaderWrite(output, &conn, pathLen, inheritPathLen);

    path.toWCharArray((wchar_t *) (output + DriverCommon::logConnHeaderSize(isIPv6)));
    inheritPath.toWCharArray(
            (wchar_t *) (output + DriverCommon::logConnInheritPathOffset(pathLen, isIPv6)));

    m_top += entrySize;
}

void LogBuffer::readEntryConn(LogEntryConn *logEntry)
{
    Q_ASSERT(m_offset < m_top);

    const char *input = this->input();

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
    logEntry->setKernelPath(path);

    const int entrySize = int(DriverCommon::logConnSize(pathLen, inheritPathLen, conn.isIPv6));
    m_offset += entrySize;
}

void LogBuffer::writeEntryProcNew(const LogEntryProcNew *logEntry)
{
    const QString path = logEntry->kernelPath();
    const quint16 pathLen = quint16(path.size()) * sizeof(wchar_t);

    const int entrySize = int(DriverCommon::logProcNewSize(pathLen));
    prepareFor(entrySize);

    char *output = this->output();

    DriverCommon::logProcNewHeaderWrite(output, logEntry->appId(), logEntry->pid(), pathLen);

    if (pathLen > 0) {
        output += DriverCommon::logProcNewHeaderSize();
        path.toWCharArray((wchar_t *) output);
    }

    m_top += entrySize;
}

void LogBuffer::readEntryProcNew(LogEntryProcNew *logEntry)
{
    Q_ASSERT(m_offset < m_top);

    const char *input = this->input();

    quint32 appId;
    quint32 pid;
    quint16 pathLen;
    DriverCommon::logProcNewHeaderRead(input, &appId, &pid, &pathLen);

    const QString path = readPath(input + DriverCommon::logProcNewHeaderSize(), pathLen);

    logEntry->setAppId(appId);
    logEntry->setPid(pid);
    logEntry->setKernelPath(path);

    const int entrySize = int(DriverCommon::logProcNewSize(pathLen));
    m_offset += entrySize;
}

void LogBuffer::readEntryStatTraf(LogEntryStatTraf *logEntry)
{
    Q_ASSERT(m_offset < m_top);

    const char *input = this->input();

    quint16 procCount;
    DriverCommon::logStatTrafHeaderRead(input, &procCount);

    logEntry->setProcCount(procCount);

    if (procCount != 0) {
        input += DriverCommon::logStatHeaderSize();
        logEntry->setProcTrafBytes(reinterpret_cast<const quint32 *>(input));
    }

    const int entrySize = int(DriverCommon::logStatSize(procCount));
    m_offset += entrySize;
}

void LogBuffer::writeEntryTime(const LogEntryTime *logEntry)
{
    const int entrySize = int(DriverCommon::logTimeSize());
    prepareFor(entrySize);

    char *output = this->output();

    DriverCommon::logTimeWrite(output, logEntry->unixTime(), logEntry->systemTimeChanged());

    m_top += entrySize;
}

void LogBuffer::readEntryTime(LogEntryTime *logEntry)
{
    Q_ASSERT(m_offset < m_top);

    const char *input = this->input();

    qint64 unixTime;
    int systemTimeChanged;
    DriverCommon::logTimeRead(input, &unixTime, &systemTimeChanged);

    logEntry->setSystemTimeChanged(systemTimeChanged != 0);
    logEntry->setUnixTime(unixTime);

    const int entrySize = int(DriverCommon::logTimeSize());
    m_offset += entrySize;
}

void LogBuffer::readEntryProcKill(LogEntryProcKill *logEntry)
{
    Q_ASSERT(m_offset < m_top);

    const char *input = this->input();

    quint32 pid;
    DriverCommon::logProcKillRead(input, &pid);

    logEntry->setPid(pid);

    const int entrySize = int(DriverCommon::logProcKillSize());
    m_offset += entrySize;
}
