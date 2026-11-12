#ifndef HOSTREFS_H
#define HOSTREFS_H

#include <QHash>
#include <QSet>
#include <QString>

class HostRange;

// The host() filters' ids by their names. The ids aren't reused, so the driver's IPs of a removed
// filter's names don't match a new filter.
class HostRefs
{
public:
    enum Source : qint8 { SourceRules = 0, SourceConf, SourceCount };

    bool isChanged() const { return m_changed; }
    void resetChanged() { m_changed = false; }

    // The ids by the names' key
    const QHash<QString, quint32> &hostIds() const { return m_hostIds; }

    // Starts collecting all host() filters of the source
    void beginSource(Source source);

    // On success the source has the collected host() filters only, then the ids of the names,
    // which no source's host() filter has, are removed
    void endSource(bool ok);

    // Returns the names' id, a new one for new names. Out of collecting it's a Program's filter.
    quint32 hostId(const HostRange &hostRange);

    static QString namesKey(const HostRange &hostRange);

private:
    void removeUnusedIds();

private:
    bool m_changed = false;
    bool m_collecting = false;

    Source m_source = SourceConf;

    quint32 m_lastHostId = 0;

    QHash<QString, quint32> m_hostIds;

    QSet<QString> m_sourceKeys[SourceCount];
    QSet<QString> m_collectedKeys;
};

#endif // HOSTREFS_H
