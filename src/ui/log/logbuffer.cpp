#include "logbuffer.h"

#include "fortcommon.h"
#include "logentryblocked.h"

LogBuffer::LogBuffer(int bufferSize, QObject *parent) :
    QObject(parent),
    m_top(0),
    m_offset(0),
    m_array(bufferSize ? bufferSize : FortCommon::bufferSize(),
            Qt::Uninitialized)
{
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

LogEntry::LogType LogBuffer::readType()
{
    if (m_offset >= m_top)
        return LogEntry::TypeNone;

    const char *input = this->input();

    const quint32 type = FortCommon::logType(input);

    return static_cast<LogEntry::LogType>(type);
}

int LogBuffer::write(const LogEntryBlocked *logEntry)
{
    const QString path = logEntry->kernelPath();
    const int pathLen = path.size() * sizeof(wchar_t);

    const int entrySize = FortCommon::logBlockedSize(pathLen);
    prepareFor(entrySize);

    char *output = this->output();

    FortCommon::logBlockedHeaderWrite(output, logEntry->ip(),
                                      logEntry->pid(), pathLen);
    output += FortCommon::logBlockedHeaderSize();

    if (pathLen) {
        path.toWCharArray((wchar_t *) output);
    }

    m_top += entrySize;

    return entrySize;
}

int LogBuffer::read(LogEntryBlocked *logEntry)
{
    if (m_offset >= m_top)
        return 0;

    const char *input = this->input();

    quint32 ip, pid, pathLen;
    FortCommon::logBlockedHeaderRead(input, &ip, &pid, &pathLen);

    QString path;
    if (pathLen) {
        input += FortCommon::logBlockedHeaderSize();
        path = QString::fromWCharArray((const wchar_t *) input,
                                       pathLen / sizeof(wchar_t));
    }

    logEntry->setIp(ip);
    logEntry->setPid(pid);
    logEntry->setKernelPath(path);

    const int entrySize = FortCommon::logBlockedSize(pathLen);
    m_offset += entrySize;

    return entrySize;
}
