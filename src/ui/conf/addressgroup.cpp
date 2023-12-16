#include "addressgroup.h"

AddressGroup::AddressGroup(QObject *parent) : QObject(parent) { }

void AddressGroup::setIncludeAll(bool includeAll)
{
    if (m_includeAll != includeAll) {
        m_includeAll = includeAll;
        setEdited(true);
    }
}

void AddressGroup::setExcludeAll(bool excludeAll)
{
    if (m_excludeAll != excludeAll) {
        m_excludeAll = excludeAll;
        setEdited(true);
    }
}

void AddressGroup::setIncludeZones(quint32 v)
{
    if (m_includeZones != v) {
        m_includeZones = v;
        setEdited(true);
    }
}

void AddressGroup::setExcludeZones(quint32 v)
{
    if (m_excludeZones != v) {
        m_excludeZones = v;
        setEdited(true);
    }
}

void AddressGroup::setIncludeText(const QString &includeText)
{
    if (m_includeText != includeText) {
        m_includeText = includeText;
        setEdited(true);
    }
}

void AddressGroup::setExcludeText(const QString &excludeText)
{
    if (m_excludeText != excludeText) {
        m_excludeText = excludeText;
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
