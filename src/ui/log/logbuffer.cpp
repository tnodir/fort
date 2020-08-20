#include "logbuffer.h"

#include "fortcommon.h"
#include "logentryblocked.h"
#include "logentryprocnew.h"
#include "logentrystattraf.h"

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

    FortCommon::logBlockedHeaderWrite(output, logEntry->blocked(), logEntry->ip(), logEntry->port(),
            logEntry->proto(), logEntry->pid(), pathLen);
    output += FortCommon::logBlockedHeaderSize();

    if (pathLen) {
        path.toWCharArray((wchar_t *) output);
    }

    m_top += entrySize;
}

void LogBuffer::readEntryBlocked(LogEntryBlocked *logEntry)
{
    Q_ASSERT(m_offset < m_top);

    const char *input = this->input();

    int blocked;
    quint8 proto;
    quint16 port;
    quint32 ip, pid, pathLen;
    FortCommon::logBlockedHeaderRead(input, &blocked, &ip, &port, &proto, &pid, &pathLen);

    QString path;
    if (pathLen) {
        input += FortCommon::logBlockedHeaderSize();
        path = QString::fromWCharArray((const wchar_t *) input, pathLen / int(sizeof(wchar_t)));
    }

    logEntry->setBlocked(blocked);
    logEntry->setIp(ip);
    logEntry->setPort(port);
    logEntry->setProto(proto);
    logEntry->setPid(pid);
    logEntry->setKernelPath(path);

    const int entrySize = int(FortCommon::logBlockedSize(pathLen));
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
    output += FortCommon::logProcNewHeaderSize();

    if (pathLen) {
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

    qint64 unixTime;
    quint16 procCount;
    FortCommon::logStatTrafHeaderRead(input, &unixTime, &procCount);

    logEntry->setProcCount(procCount);
    logEntry->setUnixTime(unixTime);

    if (procCount != 0) {
        input += FortCommon::logStatHeaderSize();
        logEntry->setProcTrafBytes(reinterpret_cast<const quint32 *>(input));
    }

    const int entrySize = int(FortCommon::logStatSize(procCount));
    m_offset += entrySize;
}
