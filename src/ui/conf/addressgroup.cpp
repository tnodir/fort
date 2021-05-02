#include "addressgroup.h"

AddressGroup::AddressGroup(QObject *parent) :
    QObject(parent), m_edited(false), m_includeAll(true), m_excludeAll(false)
{
}

void AddressGroup::addIncludeZone(int zoneId)
{
    addZone(m_includeZones, zoneId);
}

void AddressGroup::removeIncludeZone(int zoneId)
{
    removeZone(m_includeZones, zoneId);
}

void AddressGroup::addExcludeZone(int zoneId)
{
    addZone(m_excludeZones, zoneId);
}

void AddressGroup::removeExcludeZone(int zoneId)
{
    removeZone(m_excludeZones, zoneId);
}

void AddressGroup::addZone(quint32 &zones, int zoneId)
{
    zones |= (quint32(1) << (zoneId - 1));

    setEdited(true);
}

void AddressGroup::removeZone(quint32 &zones, int zoneId)
{
    zones &= ~(quint32(1) << (zoneId - 1));

    setEdited(true);
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

void AddressGroup::copy(const AddressGroup &o)
{
    m_edited = o.edited();

    m_includeAll = o.includeAll();
    m_excludeAll = o.excludeAll();

    m_id = o.id();

    m_includeZones = o.includeZones();
    m_excludeZones = o.excludeZones();

    m_includeText = o.includeText();
    m_excludeText = o.excludeText();
}

QVariant AddressGroup::toVariant() const
{
    QVariantMap map;

    map["edited"] = edited();

    map["includeAll"] = includeAll();
    map["excludeAll"] = excludeAll();

    map["id"] = id();

    map["includeZones"] = includeZones();
    map["excludeZones"] = excludeZones();

    map["includeText"] = includeText();
    map["excludeText"] = excludeText();

    return map;
}

void AddressGroup::fromVariant(const QVariant &v)
{
    const QVariantMap map = v.toMap();

    m_edited = map["edited"].toBool();

    m_includeAll = map["includeAll"].toBool();
    m_excludeAll = map["excludeAll"].toBool();

    m_id = map["id"].toLongLong();

    m_includeZones = map["includeZones"].toUInt();
    m_excludeZones = map["excludeZones"].toUInt();

    m_includeText = map["includeText"].toString();
    m_excludeText = map["excludeText"].toString();
}
