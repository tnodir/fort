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

void HostRefs::addHostFilter(const HostRange &hostRange)
{
    collectedRefs().keys.insert(namesKey(hostRange));
}

void HostRefs::addZones(quint32 zonesMask)
{
    collectedRefs().zonesMask |= zonesMask;
}

HostMatcher HostRefs::hostMatcher() const
{
    HostMatcher hostMatcher;
    HostRange hostRange;

    for (const QString &key : m_hostKeys) {
        if (hostRange.fromList(StringUtil::splitView(key, QLatin1Char(namesSeparator)))) {
            hostMatcher.addHostFilter(hostRange);
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

    setHostKeys(usedRefs.keys);

    setZonesMask(usedRefs.zonesMask);
}

void HostRefs::setHostKeys(const QSet<QString> &hostKeys)
{
    if (m_hostKeys == hostKeys)
        return;

    m_hostKeys = hostKeys;
    m_changed = true;

    updateNames();
}

void HostRefs::updateNames()
{
    QSet<QString> names;

    for (const QString &key : std::as_const(m_hostKeys)) {
        for (const QStringView name : StringUtil::splitView(key, QLatin1Char(namesSeparator))) {
            names.insert(name.toString());
        }
    }

    if (!m_names.contains(names)) {
        ++m_addedNamesCount;
    }

    m_names = names;
}

void HostRefs::setZonesMask(quint32 zonesMask)
{
    if (m_zonesMask == zonesMask)
        return;

    m_zonesMask = zonesMask;
    m_changed = true;
}
