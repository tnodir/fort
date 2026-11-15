#ifndef HOSTREFS_H
#define HOSTREFS_H

#include <QHash>
#include <QSet>
#include <QString>

#include <util/net/hostmatcher.h>

class HostRange;

// The driver's conf's references, which the DNS answers' IPs are added for: the host() filters' ids
// by their names and the Zones. The ids aren't reused, so the driver's IPs of a removed filter's
// names don't match a new filter. The Programs' references are kept by their paths, as the driver
// keeps the Programs: a Program written alone replaces its own.
class HostRefs
{
public:
    enum Source : qint8 { SourceRules = 0, SourceConf, SourceCount };

    // Whether the ids or the Zones changed
    bool isChanged() const { return m_changed; }
    void resetChanged() { m_changed = false; }

    // The ids by the names' key
    const QHash<QString, quint32> &hostIds() const { return m_hostIds; }

    // The Zones referenced by any source
    quint32 zonesMask() const { return m_zonesMask; }

    // Starts collecting all host() filters and Zones of the source
    void beginSource(Source source);

    // On success the source has the collected host() filters and Zones only, then the ids of the
    // names, which no source's host() filter has, are removed
    void endSource(bool ok);

    // Starts collecting a Program's host() filters and Zones: of the conf's source, else of the
    // Program written alone
    void beginApp(const QString &appPath);

    // On success the Program has the collected host() filters and Zones only. Written alone, then
    // the unused ids are removed.
    void endApp(bool ok);

    // Returns the names' id, a new one for new names. The names are collected only between the
    // begin*() and end*() calls, as the Zones are.
    quint32 hostId(const HostRange &hostRange);

    void addZones(quint32 zonesMask);

    // The host() filters' names with their ids
    HostMatcher hostMatcher() const;

    static QString namesKey(const HostRange &hostRange);

private:
    struct Refs
    {
        bool isEmpty() const { return zonesMask == 0 && keys.isEmpty(); }

        quint32 zonesMask = 0;
        QSet<QString> keys;
    };

    struct SourceRefs
    {
        Refs refs; // of the Rules or of the address groups
        QHash<QString, Refs> appsRefs; // of the Programs by their paths
    };

    Refs &collectedRefs() { return m_collectingApp ? m_collectedApp : m_collected.refs; }

    static void setAppRefs(
            QHash<QString, Refs> &appsRefs, const QString &appPath, const Refs &refs);

    static void addRefs(Refs &usedRefs, const Refs &refs);
    static void addSourceRefs(Refs &usedRefs, const SourceRefs &sourceRefs);

    void updateRefs();

    void removeUnusedIds(const QSet<QString> &usedKeys);

    void setZonesMask(quint32 zonesMask);

private:
    bool m_changed = false;
    bool m_collectingSource = false;
    bool m_collectingApp = false;

    Source m_source = SourceConf;

    quint32 m_lastHostId = 0;
    quint32 m_zonesMask = 0;

    QString m_appPath;

    QHash<QString, quint32> m_hostIds;

    Refs m_collectedApp;
    SourceRefs m_collected;
    SourceRefs m_sources[SourceCount];
};

#endif // HOSTREFS_H
