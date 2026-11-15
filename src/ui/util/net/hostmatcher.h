#ifndef HOSTMATCHER_H
#define HOSTMATCHER_H

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QVector>

class HostRange;

struct HostMatch
{
    bool isEmpty() const { return zonesMask == 0 && nameIds.isEmpty(); }

    void add(const HostMatch &other);

    quint32 zonesMask = 0;
    QVector<quint64> nameIds; // the host() filters' names' ids
};

// The names of the host() filters with their ids
class HostMatcher
{
public:
    void addHostFilter(const HostRange &hostRange);

    // The host() filters' names listing the name: the name itself, a "*." wildcard of its parent
    // domain or another mask
    HostMatch match(QStringView name) const;
    // By the name's HostRange::normalizedKey()
    HostMatch matchKey(const QByteArray &key) const;

private:
    // The names are ASCII by HostRange: in Latin-1 they take half the memory
    QHash<QByteArray, quint64> m_names;
    QHash<QByteArray, quint64> m_wildcardNames; // the parent domains of the "*." wildcards
    QHash<QByteArray, quint64> m_patternNames; // the names with other masks: HostPattern
};

#endif // HOSTMATCHER_H
