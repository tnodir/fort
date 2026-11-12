#include "hostrefs.h"

#include <util/net/hostrange.h>
#include <util/stringutil.h>

namespace {

inline constexpr char namesSeparator = ',';

}

void HostRefs::beginSource(Source source)
{
    m_collectingSource = true;
    m_source = source;

    m_collected = {};
}

void HostRefs::endSource(bool ok)
{
    m_collectingSource = false;

    if (ok) {
        m_sources[m_source] = m_collected;
        updateRefs();
    }

    m_collected = {};
}

void HostRefs::beginApp(const QString &appPath)
{
    m_collectingApp = true;
    m_appPath = appPath;

    m_collectedApp = {};
}

void HostRefs::endApp(bool ok)
{
    m_collectingApp = false;

    if (ok) {
        if (m_collectingSource) {
            setAppRefs(m_collected.appsRefs, m_appPath, m_collectedApp);
        } else {
            setAppRefs(m_sources[SourceConf].appsRefs, m_appPath, m_collectedApp);
            updateRefs();
        }
    }

    m_appPath.clear();
    m_collectedApp = {};
}

quint32 HostRefs::hostId(const HostRange &hostRange)
{
    const QString key = namesKey(hostRange);

    collectedRefs().keys.insert(key);

    quint32 &hostId = m_hostIds[key];

    if (hostId == 0) {
        hostId = ++m_lastHostId;
        m_changed = true;
    }

    return hostId;
}

void HostRefs::addZones(quint32 zonesMask)
{
    collectedRefs().zonesMask |= zonesMask;
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

void HostRefs::setAppRefs(QHash<QString, Refs> &appsRefs, const QString &appPath, const Refs &refs)
{
    if (refs.isEmpty()) {
        appsRefs.remove(appPath);
    } else {
        appsRefs.insert(appPath, refs);
    }
}

void HostRefs::addRefs(Refs &usedRefs, const Refs &refs)
{
    usedRefs.zonesMask |= refs.zonesMask;
    usedRefs.keys.unite(refs.keys);
}

void HostRefs::addSourceRefs(Refs &usedRefs, const SourceRefs &sourceRefs)
{
    addRefs(usedRefs, sourceRefs.refs);

    for (const Refs &appRefs : sourceRefs.appsRefs) {
        addRefs(usedRefs, appRefs);
    }
}

void HostRefs::updateRefs()
{
    Refs usedRefs;

    for (const SourceRefs &sourceRefs : m_sources) {
        addSourceRefs(usedRefs, sourceRefs);
    }

    removeUnusedIds(usedRefs.keys);

    setZonesMask(usedRefs.zonesMask);
}

void HostRefs::removeUnusedIds(const QSet<QString> &usedKeys)
{
    for (auto it = m_hostIds.begin(); it != m_hostIds.end();) {
        if (usedKeys.contains(it.key())) {
            ++it;
        } else {
            it = m_hostIds.erase(it);
            m_changed = true;
        }
    }
}

void HostRefs::setZonesMask(quint32 zonesMask)
{
    if (m_zonesMask == zonesMask)
        return;

    m_zonesMask = zonesMask;
    m_changed = true;
}
