#include "logentry.h"

#include "../util/fileutil.h"
#include "../util/netutil.h"
#include "../util/processinfo.h"

LogEntry::LogEntry(quint32 ip, quint32 pid,
                   const QString &dosPath,
                   QObject *parent) :
    QObject(parent),
    m_ip(ip),
    m_pid(pid),
    m_dosPath(dosPath)
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

void LogEntry::setDosPath(const QString &dosPath)
{
    if (m_dosPath != dosPath) {
        m_dosPath = dosPath;
        emit dosPathChanged();
    }
}

QString LogEntry::ipText() const
{
    return NetUtil::ip4ToText(m_ip);
}

QString LogEntry::path() const
{
    QString dosPath = m_dosPath;
    if (dosPath.isEmpty()) {
        const ProcessInfo pi(m_pid);
        dosPath = pi.dosPath();
    }

    return FileUtil::dosPathToPath(dosPath);
}
