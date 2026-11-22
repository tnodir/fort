#include "hostzonesfill.h"

#include <algorithm>

#include <conf/zone.h>
#include <task/taskinfozonedownloader.h>
#include <util/fileutil.h>

#include "hostzonesdb.h"

HostZonesFill::HostZonesFill(HostZonesDb &db) : m_db(db) { }

bool HostZonesFill::update(const HostZonesSource &source)
{
    updateFilledMask(source);

    const quint32 enabledMask = source.enabledMask & m_filledMask;

    if (m_db.enabledMask() == enabledMask)
        return false;

    m_db.setEnabledMask(enabledMask);

    return true;
}

void HostZonesFill::clear()
{
    m_filledMask = 0;
}

void HostZonesFill::removeZone(int zoneId)
{
    const quint32 zoneBit = Zone::zoneBit(zoneId);

    m_filledMask &= ~zoneBit;
    m_db.setEnabledMask(m_db.enabledMask() & ~zoneBit);
}

bool HostZonesFill::hasZonesToFill(const HostZonesSource &source, quint32 zonesMask) const
{
    return hasUnfilledZones(source, zonesMask, /*cached=*/true);
}

bool HostZonesFill::hasZonesUncached(const HostZonesSource &source, quint32 zonesMask) const
{
    return hasUnfilledZones(source, zonesMask, /*cached=*/false);
}

bool HostZonesFill::hasUnfilledZones(
        const HostZonesSource &source, quint32 zonesMask, bool cached) const
{
    const quint32 unfilledMask = zonesMask & ~m_filledMask;
    if (unfilledMask == 0)
        return false;

    return std::any_of(source.zones.begin(), source.zones.end(), [&](const HostZoneSource &zone) {
        return (unfilledMask & Zone::zoneBit(zone.zoneId)) != 0
                && FileUtil::fileExists(zone.cacheFilePath) == cached;
    });
}

// A Zone's names are its own since they are of its cache file, also its old names after its update
// till the new ones are written: the DB's names of a Zone removed meanwhile (e.g. while the
// forwarder was stopped), whose id is reused, aren't matched
void HostZonesFill::updateFilledMask(const HostZonesSource &source)
{
    if (!m_db.isOpened())
        return;

    m_filledMask |= actualZonesMask(source);
    m_filledMask &= source.hostsMask & m_db.zonesMask();
}

// The Zones having the names of their cache files in the DB
quint32 HostZonesFill::actualZonesMask(const HostZonesSource &source) const
{
    quint32 zonesMask = 0;

    for (const HostZoneSource &zone : source.zones) {
        if (m_db.isZoneActual(zone.zoneId, zone.binChecksum, zone.subdomains)) {
            zonesMask |= Zone::zoneBit(zone.zoneId);
        }
    }

    return zonesMask;
}
