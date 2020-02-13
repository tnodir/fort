#include "addressgroup.h"

AddressGroup::AddressGroup(QObject *parent) :
    QObject(parent),
    m_edited(false),
    m_includeAll(true),
    m_excludeAll(false)
{
}

void AddressGroup::setIncludeAll(bool includeAll)
{
    if (m_includeAll != includeAll) {
        m_includeAll = includeAll;
        emit includeAllChanged();

        setEdited(true);
    }
}

void AddressGroup::setExcludeAll(bool excludeAll)
{
    if (m_excludeAll != excludeAll) {
        m_excludeAll = excludeAll;
        emit excludeAllChanged();

        setEdited(true);
    }
}

void AddressGroup::setIncludeText(const QString &includeText)
{
    if (m_includeText != includeText) {
        m_includeText = includeText;
        emit includeTextChanged();

        setEdited(true);
    }
}

void AddressGroup::setExcludeText(const QString &excludeText)
{
    if (m_excludeText != excludeText) {
        m_excludeText = excludeText;
        emit excludeTextChanged();

        setEdited(true);
    }
}

void AddressGroup::addIncludeZone(int zoneId, bool sorting)
{
    addZone(m_includeZones, zoneId, sorting);
}

void AddressGroup::removeIncludeZone(int zoneId)
{
    removeZone(m_includeZones, zoneId);
}

void AddressGroup::addExcludeZone(int zoneId, bool sorting)
{
    addZone(m_excludeZones, zoneId, sorting);
}

void AddressGroup::removeExcludeZone(int zoneId)
{
    removeZone(m_excludeZones, zoneId);
}

void AddressGroup::addZone(QVector<int> &zones, int zoneId, bool sorting)
{
    zones.append(zoneId);

    if (sorting) {
        std::sort(zones.begin(), zones.end());
    }

    setEdited(true);
}

void AddressGroup::removeZone(QVector<int> &zones, int zoneId)
{
    zones.removeOne(zoneId);

    setEdited(true);
}

void AddressGroup::copy(const AddressGroup &o)
{
    m_edited = o.edited();

    m_includeAll = o.includeAll();
    m_excludeAll = o.excludeAll();

    m_id = o.id();

    m_includeText = o.includeText();
    m_excludeText = o.excludeText();

    m_includeZones = o.includeZones();
    m_excludeZones = o.excludeZones();
}
