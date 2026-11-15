#include "hostmatcher.h"

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

void HostMatcher::addHostFilter(quint32 hostId, const HostRange &hostRange)
{
    const HostMatch match = { .hostIds = { hostId } };

    addNames(m_names, hostRange.names(), match);
    addNames(m_wildcardNames, hostRange.wildcardNames(), match);
    addNames(m_patternNames, hostRange.patternNames(), match);
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
