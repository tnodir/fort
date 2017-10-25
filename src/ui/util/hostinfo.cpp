#include "hostinfo.h"

HostInfo::HostInfo(QObject *parent) :
    QObject(parent)
{
}

HostInfo::~HostInfo()
{
    abortHostLookups();
}

void HostInfo::lookupHost(const QString &name)
{
//    const int lookupId = QHostInfo::lookupHost(
//                name, this, &HostInfo::handleLookedupHost);
    const int lookupId = -1;

    m_lookupIds.insert(lookupId, name);
}

void HostInfo::abortHostLookup(int lookupId)
{
    m_lookupIds.remove(lookupId);

//    QHostInfo::abortHostLookup(lookupId);
}

void HostInfo::abortHostLookups()
{
    foreach (const int lookupId, m_lookupIds.keys()) {
        abortHostLookup(lookupId);
    }
}

//void HostInfo::handleLookedupHost(const QHostInfo &info)
//{
//    const int lookupId = info.lookupId();
//    const QString name = m_lookupIds.value(lookupId);
//    const bool success = (info.error() == QHostInfo::NoError);

//    m_info = info;

//    m_lookupIds.remove(lookupId);

//    emit hostLookedup(name, success);
//}
