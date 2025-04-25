#include "logbuffer.h"

#include <driver/drivercommon.h>

#include "logentryapp.h"
#include "logentryconn.h"
#include "logentryprocnew.h"
#include "logentrystattraf.h"
#include "logentrytime.h"

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

    QString path;
    if (pathLen > 0) {
        input += DriverCommon::logAppHeaderSize();
        path = QString::fromWCharArray((const wchar_t *) input, pathLen / int(sizeof(wchar_t)));
    }

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

    const bool isIPv6 = logEntry->isIPv6();
    const int entrySize = int(DriverCommon::logConnSize(pathLen, isIPv6));
    prepareFor(entrySize);

    char *output = this->output();

    const FORT_CONF_META_CONN conn = {
        .inbound = logEntry->inbound(),
        .isIPv6 = logEntry->isIPv6(),
        .inherited = logEntry->inherited(),
        .reason = logEntry->reason(),
        .ip_proto = logEntry->ipProto(),
        .zone_id = logEntry->zoneId(),
        .rule_id = logEntry->ruleId(),
        .local_port = logEntry->localPort(),
        .remote_port = logEntry->remotePort(),
        .process_id = logEntry->pid(),
        .local_ip = logEntry->localIp(),
        .remote_ip = logEntry->remoteIp(),
    };

    DriverCommon::logConnHeaderWrite(output, &conn, pathLen);

    if (pathLen) {
        output += DriverCommon::logConnHeaderSize(logEntry->isIPv6());
        path.toWCharArray((wchar_t *) output);
    }

    m_top += entrySize;
}

void LogBuffer::readEntryConn(LogEntryConn *logEntry)
{
    Q_ASSERT(m_offset < m_top);

    const char *input = this->input();

    FORT_CONF_META_CONN conn;
    quint16 pathLen;

    DriverCommon::logConnHeaderRead(input, &conn, &pathLen);

    QString path;
    if (pathLen > 0) {
        input += DriverCommon::logConnHeaderSize(conn.isIPv6);
        path = QString::fromWCharArray((const wchar_t *) input, pathLen / int(sizeof(wchar_t)));
    }

    logEntry->setBlocked(conn.blocked);
    logEntry->setIsIPv6(conn.isIPv6);
    logEntry->setInbound(conn.inbound);
    logEntry->setInherited(conn.inherited);
    logEntry->setReason(conn.reason);
    logEntry->setIpProto(conn.ip_proto);
    logEntry->setZoneId(conn.zone_id);
    logEntry->setRuleId(conn.rule_id);
    logEntry->setLocalPort(conn.local_port);
    logEntry->setRemotePort(conn.remote_port);
    logEntry->setLocalIp(conn.local_ip);
    logEntry->setRemoteIp(conn.remote_ip);
    logEntry->setPid(conn.process_id);
    logEntry->setKernelPath(path);

    const int entrySize = int(DriverCommon::logConnSize(pathLen, conn.isIPv6));
    m_offset += entrySize;
}

void LogBuffer::writeEntryProcNew(const LogEntryProcNew *logEntry)
{
    const QString path = logEntry->kernelPath();
    const quint16 pathLen = quint16(path.size()) * sizeof(wchar_t);

    const int entrySize = int(DriverCommon::logProcNewSize(pathLen));
    prepareFor(entrySize);

    char *output = this->output();

    DriverCommon::logProcNewHeaderWrite(output, logEntry->pid(), pathLen);

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

    quint32 pid;
    quint16 pathLen;
    DriverCommon::logProcNewHeaderRead(input, &pid, &pathLen);

    QString path;
    if (pathLen > 0) {
        input += DriverCommon::logProcNewHeaderSize();
        path = QString::fromWCharArray((const wchar_t *) input, pathLen / sizeof(wchar_t));
    }

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

    DriverCommon::logTimeWrite(output, logEntry->systemTimeChanged(), logEntry->unixTime());

    m_top += entrySize;
}

void LogBuffer::readEntryTime(LogEntryTime *logEntry)
{
    Q_ASSERT(m_offset < m_top);

    const char *input = this->input();

    int systemTimeChanged;
    qint64 unixTime;
    DriverCommon::logTimeRead(input, &systemTimeChanged, &unixTime);

    logEntry->setSystemTimeChanged(systemTimeChanged != 0);
    logEntry->setUnixTime(unixTime);

    const int entrySize = int(DriverCommon::logTimeSize());
    m_offset += entrySize;
}
