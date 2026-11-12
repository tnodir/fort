#ifndef HOSTMATCHER_H
#define HOSTMATCHER_H

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QVector>

class HostRange;

struct HostMatch
{
    bool isEmpty() const { return zonesMask == 0 && hostIds.isEmpty(); }

    void add(const HostMatch &other);

    quint32 zonesMask = 0;
    QVector<quint32> hostIds; // the host() filters' ids
};

// The names of the Hosts Zones and of the host() filters
class HostMatcher
{
public:
    bool isEmpty() const;

    void addZone(int zoneId, const HostRange &hostRange);
    void addHostFilter(quint32 hostId, const HostRange &hostRange);

    // Returns whether the Zone was listed
    bool removeZone(int zoneId);

    // The Zones and host() filters listing the name by itself, by a "*." wildcard of its parent
    // domain or by another mask
    HostMatch match(QStringView name) const;

private:
    void addMatch(const HostRange &hostRange, const HostMatch &match);

private:
    // The names are ASCII by HostRange: in Latin-1 they take half the memory
    QHash<QByteArray, HostMatch> m_names;
    QHash<QByteArray, HostMatch> m_wildcardNames; // the parent domains of the "*." wildcards
    QHash<QByteArray, HostMatch> m_patternNames; // the names with other masks: HostPattern
};

#endif // HOSTMATCHER_H
