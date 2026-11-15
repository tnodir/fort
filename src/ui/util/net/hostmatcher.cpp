#include "hostmatcher.h"

#include "hostpattern.h"
#include "hostrange.h"

namespace {

using name_id_func_t = quint64 (*)(QStringView name);

void addNames(
        QHash<QByteArray, quint64> &nameIds, const QSet<QString> &names, name_id_func_t nameId)
{
    for (const auto &name : names) {
        nameIds.insert(name.toLatin1(), nameId(name));
    }
}

QByteArray rawKey(QByteArrayView name)
{
    return QByteArray::fromRawData(name.constData(), name.size());
}

void addNameId(HostMatch &match, const QHash<QByteArray, quint64> &nameIds, QByteArrayView name)
{
    const auto it = nameIds.constFind(rawKey(name));

    if (it != nameIds.constEnd()) {
        match.nameIds.append(it.value());
    }
}

}

void HostMatch::add(const HostMatch &other)
{
    zonesMask |= other.zonesMask;

    for (const quint64 nameId : other.nameIds) {
        if (!nameIds.contains(nameId)) {
            nameIds.append(nameId);
        }
    }
}

void HostMatcher::addHostFilter(const HostRange &hostRange)
{
    addNames(m_names, hostRange.names(), &HostRange::nameId);
    addNames(m_wildcardNames, hostRange.wildcardNames(), &HostRange::wildcardNameId);
    addNames(m_patternNames, hostRange.patternNames(), &HostRange::nameId);
}

HostMatch HostMatcher::match(QStringView name) const
{
    return matchKey(HostRange::normalizedKey(name));
}

HostMatch HostMatcher::matchKey(const QByteArray &key) const
{
    HostMatch match;

    HostRange::walkName(
            key, [&](QByteArrayView nameKey) { addNameId(match, m_names, nameKey); },
            [&](QByteArrayView parentKey) { addNameId(match, m_wildcardNames, parentKey); });

    for (auto it = m_patternNames.cbegin(); it != m_patternNames.cend(); ++it) {
        if (HostPattern::match(it.key(), key)) {
            match.nameIds.append(it.value());
        }
    }

    return match;
}
