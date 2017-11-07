#include "hostinfo.h"

#include <QThreadPool>

#include "hostinfoworker.h"

HostInfo::HostInfo(QObject *parent) :
    QObject(parent),
    m_worker(nullptr)
{
}

HostInfo::~HostInfo()
{
    abort();
}

void HostInfo::setupWorker()
{
    m_worker = new HostInfoWorker();  // autoDelete = true

    connect(m_worker, &HostInfoWorker::lookupFinished,
            this, &HostInfo::lookupFinished);

    QThreadPool::globalInstance()->start(m_worker);
}

void HostInfo::lookupHost(const QString &address)
{
    if (!m_worker) {
        setupWorker();
    }

    m_worker->lookupHost(address);
}

void HostInfo::clear()
{
    if (m_worker) {
        m_worker->clear();
    }
}

void HostInfo::abort()
{
    if (!m_worker) return;

    m_worker->disconnect(this);

    m_worker->abort();
    m_worker = nullptr;
}
