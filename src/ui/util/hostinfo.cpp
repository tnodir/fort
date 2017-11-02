#include "hostinfo.h"

#include "netutil.h"

HostInfo::HostInfo(QObject *parent) :
    QThread(parent),
    m_cancelled(false)
{
}

HostInfo::~HostInfo()
{
    cancel();
    wait();
}

void HostInfo::lookupHost(const QString &address)
{
    QMutexLocker locker(&m_mutex);

    m_queue.enqueue(address);

    m_waitCondition.wakeOne();

    if (!isRunning()) {
        start();
    }
}

void HostInfo::clear()
{
    QMutexLocker locker(&m_mutex);

    m_queue.clear();
}

void HostInfo::cancel()
{
    if (m_cancelled) return;

    QMutexLocker locker(&m_mutex);

    m_cancelled = true;

    m_waitCondition.wakeOne();
}

void HostInfo::run()
{
    QSysInfo::machineHostName();  // initialize ws2_32.dll

    do {
        const QString address = dequeueAddress();

        if (!address.isEmpty()) {
            const QString hostName = NetUtil::getHostName(address);

            if (m_cancelled) break;

            emit lookupFinished(address, hostName);
        }
    } while (!m_cancelled);
}

QString HostInfo::dequeueAddress()
{
    QMutexLocker locker(&m_mutex);

    while (m_queue.isEmpty()) {
        if (m_cancelled)
            return QString();

        m_waitCondition.wait(&m_mutex);
    }

    return m_queue.dequeue();
}
