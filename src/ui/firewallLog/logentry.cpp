#include "logentry.h"

LogEntry::LogEntry(QObject *parent) :
    QObject(parent)
{
}

void LogEntry::setIp(quint32 ip)
{
    if (m_ip != ip) {
        m_ip = ip;
        emit ipChanged();
    }
}

void LogEntry::setPid(quint32 pid)
{
    if (m_pid != pid) {
        m_pid = pid;
        emit pidChanged();
    }
}

void LogEntry::setPath(const QString &path)
{
    if (m_path != path) {
        m_path = path;
        emit pathChanged();
    }
}
