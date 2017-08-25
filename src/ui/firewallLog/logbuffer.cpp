#include "logbuffer.h"

#include "fortcommon.h"
#include "logentry.h"

LogBuffer::LogBuffer(int bufferSize, QObject *parent) :
    QObject(parent),
    m_top(0),
    m_offset(0),
    m_array(bufferSize, Qt::Uninitialized)
{
}

void LogBuffer::prepareFor(int len)
{
    const int newSize = m_top + len;

    if (newSize > m_array.size()) {
        m_array.resize(newSize);
    }
}

int LogBuffer::write(const LogEntry &logEntry)
{
    const QString path = logEntry.path();
    const int pathLen = path.size() * sizeof(wchar_t);

    const int entrySize = FortCommon::logSize(pathLen);
    prepareFor(entrySize);

    char *output = m_array.data() + m_top;

    FortCommon::logHeaderWrite(output, logEntry.ip(), logEntry.pid(), pathLen);
    output += FortCommon::logHeaderSize();

    if (pathLen) {
        path.toWCharArray((wchar_t *) output);
    }

    m_top += entrySize;

    return entrySize;
}

int LogBuffer::read(LogEntry &logEntry)
{
    if (m_offset >= m_top)
        return 0;

    const char *input = m_array.constData() + m_offset;

    quint32 ip, pid, pathLen;
    FortCommon::logHeaderRead(input, &ip, &pid, &pathLen);

    QString path;
    if (pathLen) {
        input += FortCommon::logHeaderSize();
        path = QString::fromWCharArray((const wchar_t *) input,
                                       pathLen / sizeof(wchar_t));
    }

    logEntry.setIp(ip);
    logEntry.setPid(pid);
    logEntry.setPath(path);

    const int entrySize = FortCommon::logSize(pathLen);
    m_offset += entrySize;

    return entrySize;
}
