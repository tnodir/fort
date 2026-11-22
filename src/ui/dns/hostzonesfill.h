#ifndef HOSTZONESFILL_H
#define HOSTZONESFILL_H

#include <QtGlobal>

#include <util/classhelpers.h>

class HostZonesDb;
struct HostZonesSource;

// The Hosts Zones having their own names in the DB: only the enabled ones are matched, the others
// are to be filled from their cache files
class HostZonesFill
{
public:
    explicit HostZonesFill(HostZonesDb &db);
    CLASS_DELETE_COPY_MOVE(HostZonesFill)

    quint32 filledMask() const { return m_filledMask; }

    // Enables the filled Zones of the source's enabled ones; returns whether the DB's enabled Zones
    // changed
    bool update(const HostZonesSource &source);

    // Of the DB's close
    void clear();

    // The Zone's id can be reused: its names aren't matched at once
    void removeZone(int zoneId);

    // Whether the Zones of the mask have their cache files, but no own names in the DB
    bool hasZonesToFill(const HostZonesSource &source, quint32 zonesMask) const;

    // Whether the Zones of the mask have neither their cache files nor own names in the DB, e.g.
    // not updated yet
    bool hasZonesUncached(const HostZonesSource &source, quint32 zonesMask) const;

private:
    bool hasUnfilledZones(const HostZonesSource &source, quint32 zonesMask, bool cached) const;

    void updateFilledMask(const HostZonesSource &source);
    quint32 actualZonesMask(const HostZonesSource &source) const;

private:
    quint32 m_filledMask = 0;

    HostZonesDb &m_db;
};

#endif // HOSTZONESFILL_H
