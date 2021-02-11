#include "logbuffer.h"

#include "fortcommon.h"
#include "logentryblocked.h"
#include "logentryblockedip.h"
#include "logentryprocnew.h"
#include "logentrystattraf.h"
#include "logentrytime.h"

LogBuffer::LogBuffer(int bufferSize, QObject *parent) :
    QObject(parent),
    m_array(bufferSize ? bufferSize : FortCommon::bufferSize(), Qt::Initialization::Uninitialized)
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

LogEntry::LogType LogBuffer::peekEntryType()
{
    if (m_offset >= m_top)
        return LogEntry::TypeNone;

    const char *input = this->input();

    const quint32 type = FortCommon::logType(input);

    return static_cast<LogEntry::LogType>(type);
}

void LogBuffer::writeEntryBlocked(const LogEntryBlocked *logEntry)
{
    const QString path = logEntry->kernelPath();
    const quint32 pathLen = quint32(path.size()) * sizeof(wchar_t);

    const int entrySize = int(FortCommon::logBlockedSize(pathLen));
    prepareFor(entrySize);

    char *output = this->output();

    FortCommon::logBlockedHeaderWrite(output, logEntry->blocked(), logEntry->pid(), pathLen);

    if (pathLen) {
        output += FortCommon::logBlockedHeaderSize();
        path.toWCharArray((wchar_t *) output);
    }

    m_top += entrySize;
}

void LogBuffer::readEntryBlocked(LogEntryBlocked *logEntry)
{
    Q_ASSERT(m_offset < m_top);

    const char *input = this->input();

    int blocked;
    quint32 pid, pathLen;
    FortCommon::logBlockedHeaderRead(input, &blocked, &pid, &pathLen);

    QString path;
    if (pathLen) {
        input += FortCommon::logBlockedHeaderSize();
        path = QString::fromWCharArray((const wchar_t *) input, pathLen / int(sizeof(wchar_t)));
    }

    logEntry->setBlocked(blocked);
    logEntry->setPid(pid);
    logEntry->setKernelPath(path);

    const int entrySize = int(FortCommon::logBlockedSize(pathLen));
    m_offset += entrySize;
}

void LogBuffer::writeEntryBlockedIp(const LogEntryBlockedIp *logEntry)
{
    const QString path = logEntry->kernelPath();
    const quint32 pathLen = quint32(path.size()) * sizeof(wchar_t);

    const int entrySize = int(FortCommon::logBlockedIpSize(pathLen));
    prepareFor(entrySize);

    char *output = this->output();

    FortCommon::logBlockedIpHeaderWrite(output, logEntry->blockReason(), logEntry->proto(),
            logEntry->localPort(), logEntry->remotePort(), logEntry->localIp(),
            logEntry->remoteIp(), logEntry->pid(), pathLen);

    if (pathLen) {
        output += FortCommon::logBlockedIpHeaderSize();
        path.toWCharArray((wchar_t *) output);
    }

    m_top += entrySize;
}

void LogBuffer::readEntryBlockedIp(LogEntryBlockedIp *logEntry)
{
    Q_ASSERT(m_offset < m_top);

    const char *input = this->input();

    quint8 blockReason;
    quint8 proto;
    quint16 localPort;
    quint16 remotePort;
    quint32 localIp, remoteIp;
    quint32 pid, pathLen;
    FortCommon::logBlockedIpHeaderRead(input, &blockReason, &proto, &localPort, &remotePort,
            &localIp, &remoteIp, &pid, &pathLen);

    QString path;
    if (pathLen) {
        input += FortCommon::logBlockedIpHeaderSize();
        path = QString::fromWCharArray((const wchar_t *) input, pathLen / int(sizeof(wchar_t)));
    }

    logEntry->setBlockReason(blockReason);
    logEntry->setLocalIp(localIp);
    logEntry->setRemoteIp(remoteIp);
    logEntry->setLocalPort(localPort);
    logEntry->setRemotePort(remotePort);
    logEntry->setProto(proto);
    logEntry->setPid(pid);
    logEntry->setKernelPath(path);

    const int entrySize = int(FortCommon::logBlockedIpSize(pathLen));
    m_offset += entrySize;
}

void LogBuffer::writeEntryProcNew(const LogEntryProcNew *logEntry)
{
    const QString path = logEntry->kernelPath();
    const quint32 pathLen = quint32(path.size()) * sizeof(wchar_t);

    const int entrySize = int(FortCommon::logProcNewSize(pathLen));
    prepareFor(entrySize);

    char *output = this->output();

    FortCommon::logProcNewHeaderWrite(output, logEntry->pid(), pathLen);

    if (pathLen != 0) {
        output += FortCommon::logProcNewHeaderSize();
        path.toWCharArray((wchar_t *) output);
    }

    m_top += entrySize;
}

void LogBuffer::readEntryProcNew(LogEntryProcNew *logEntry)
{
    Q_ASSERT(m_offset < m_top);

    const char *input = this->input();

    quint32 pid, pathLen;
    FortCommon::logProcNewHeaderRead(input, &pid, &pathLen);

    QString path;
    if (pathLen) {
        input += FortCommon::logProcNewHeaderSize();
        path = QString::fromWCharArray((const wchar_t *) input, pathLen / int(sizeof(wchar_t)));
    }

    logEntry->setPid(pid);
    logEntry->setKernelPath(path);

    const int entrySize = int(FortCommon::logProcNewSize(pathLen));
    m_offset += entrySize;
}

void LogBuffer::readEntryStatTraf(LogEntryStatTraf *logEntry)
{
    Q_ASSERT(m_offset < m_top);

    const char *input = this->input();

    quint16 procCount;
    FortCommon::logStatTrafHeaderRead(input, &procCount);

    logEntry->setProcCount(procCount);

    if (procCount != 0) {
        input += FortCommon::logStatHeaderSize();
        logEntry->setProcTrafBytes(reinterpret_cast<const quint32 *>(input));
    }

    const int entrySize = int(FortCommon::logStatSize(procCount));
    m_offset += entrySize;
}

void LogBuffer::writeEntryTime(const LogEntryTime *logEntry)
{
    const int entrySize = int(FortCommon::logTimeSize());
    prepareFor(entrySize);

    char *output = this->output();

    FortCommon::logTimeWrite(output, logEntry->unixTime());

    m_top += entrySize;
}

void LogBuffer::readEntryTime(LogEntryTime *logEntry)
{
    Q_ASSERT(m_offset < m_top);

    const char *input = this->input();

    qint64 unixTime;
    FortCommon::logTimeRead(input, &unixTime);

    logEntry->setUnixTime(unixTime);

    const int entrySize = int(FortCommon::logTimeSize());
    m_offset += entrySize;
}
