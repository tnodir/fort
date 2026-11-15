#include "hostmatcher.h"

#include <conf/zone.h>

#include "hostpattern.h"
#include "hostrange.h"

namespace {

void addNames(
        QHash<QByteArray, HostMatch> &matches, const QSet<QString> &names, const HostMatch &match)
{
    for (const auto &name : names) {
        matches[name.toLatin1()].add(match);
    }
}

QByteArray rawKey(QByteArrayView name)
{
    return QByteArray::fromRawData(name.constData(), name.size());
}

}

void HostMatch::add(const HostMatch &other)
{
    zonesMask |= other.zonesMask;

    for (const quint32 hostId : other.hostIds) {
        if (!hostIds.contains(hostId)) {
            hostIds.append(hostId);
        }
    }
}

bool HostMatcher::isEmpty() const
{
    return m_names.isEmpty() && m_wildcardNames.isEmpty() && m_patternNames.isEmpty();
}

void HostMatcher::addZone(int zoneId, const HostRange &hostRange)
{
    addMatch(hostRange, { .zonesMask = Zone::zoneBit(zoneId) });
}

void HostMatcher::addHostFilter(quint32 hostId, const HostRange &hostRange)
{
    addMatch(hostRange, { .hostIds = { hostId } });
}

HostMatch HostMatcher::match(QStringView name) const
{
    const QByteArray key = HostRange::normalizedKey(name);

    HostMatch match;

    HostRange::walkName(
            key, [&](QByteArrayView nameKey) { match = m_names.value(rawKey(nameKey)); },
            [&](QByteArrayView parentKey) { match.add(m_wildcardNames.value(rawKey(parentKey))); });

    for (auto it = m_patternNames.cbegin(); it != m_patternNames.cend(); ++it) {
        if (HostPattern::match(it.key(), key)) {
            match.add(it.value());
        }
    }

    return match;
}

void HostMatcher::addMatch(const HostRange &hostRange, const HostMatch &match)
{
    addNames(m_names, hostRange.names(), match);
    addNames(m_wildcardNames, hostRange.wildcardNames(), match);
    addNames(m_patternNames, hostRange.patternNames(), match);
}
