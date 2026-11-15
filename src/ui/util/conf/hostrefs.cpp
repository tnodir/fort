#include "hostrefs.h"

#include <util/net/hostrange.h>
#include <util/stringutil.h>

namespace {

inline constexpr char namesSeparator = ',';

}

void HostRefs::beginSource(Source source)
{
    m_collecting = true;
    m_source = source;

    m_collectedKeys.clear();
}

void HostRefs::endSource(bool ok)
{
    m_collecting = false;

    if (ok) {
        m_sourceKeys[m_source] = m_collectedKeys;
        removeUnusedIds();
    }

    m_collectedKeys.clear();
}

quint32 HostRefs::hostId(const HostRange &hostRange)
{
    const QString key = namesKey(hostRange);

    auto &keys = m_collecting ? m_collectedKeys : m_sourceKeys[SourceConf];
    keys.insert(key);

    quint32 &hostId = m_hostIds[key];

    if (hostId == 0) {
        hostId = ++m_lastHostId;
        m_changed = true;
    }

    return hostId;
}

HostMatcher HostRefs::hostMatcher() const
{
    HostMatcher hostMatcher;
    HostRange hostRange;

    for (auto it = m_hostIds.constBegin(); it != m_hostIds.constEnd(); ++it) {
        if (hostRange.fromList(StringUtil::splitView(it.key(), QLatin1Char(namesSeparator)))) {
            hostMatcher.addHostFilter(it.value(), hostRange);
        }
    }

    return hostMatcher;
}

QString HostRefs::namesKey(const HostRange &hostRange)
{
    QStringList names;
    hostRange.toList(names);

    return names.join(QLatin1Char(namesSeparator));
}

void HostRefs::removeUnusedIds()
{
    const QSet<QString> usedKeys = m_sourceKeys[SourceRules] | m_sourceKeys[SourceConf];

    for (auto it = m_hostIds.begin(); it != m_hostIds.end();) {
        if (usedKeys.contains(it.key())) {
            ++it;
        } else {
            it = m_hostIds.erase(it);
            m_changed = true;
        }
    }
}
