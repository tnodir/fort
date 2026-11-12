#ifndef DNSHOSTBLOCKER_H
#define DNSHOSTBLOCKER_H

#include <QStringView>

#include <util/classhelpers.h>

class DnsAnswer;
class DnsQuery;
class HostZonesDb;

// The names of the Block Zones aren't found, even if a host() filter lists them
class DnsHostBlocker
{
public:
    explicit DnsHostBlocker(const HostZonesDb &hostZonesDb);
    CLASS_DELETE_COPY_MOVE(DnsHostBlocker)

    quint32 zonesMask() const { return m_zonesMask; }
    void setZonesMask(quint32 v) { m_zonesMask = v; }

    bool isQueryBlocked(const DnsQuery &query) const;
    bool isAnswerBlocked(const DnsQuery &query, const DnsAnswer &answer) const;

private:
    bool isHostBlocked(QStringView name) const;

private:
    quint32 m_zonesMask = 0;

    const HostZonesDb &m_hostZonesDb;
};

#endif // DNSHOSTBLOCKER_H
