#include "mapsettings.h"

#include <qtcompat.h>

#include "settings.h"

MapSettings::MapSettings(Settings *settings) : m_settings(settings) { }

MapSettings::MapSettings(const MapSettings &o) :
    MapWrapper(o.map()), m_settings(o.settings()), m_removedGroups(o.m_removedGroups)
{
}

QVariant MapSettings::value(const QString &key, const QVariant &defaultValue) const
{
    const QVariant v = MapWrapper::value(key);
    if (!v.isNull())
        return v;

    return settings() ? settings()->iniValue(key, defaultValue) : defaultValue;
}

void MapSettings::setValue(const QString &key, const QVariant &v, const QVariant &defaultValue)
{
    const QVariant oldValue = value(key, defaultValue);
    if (oldValue == v)
        return;

    MapWrapper::setValue(key, v, defaultValue);
}

void MapSettings::setCacheValue(const QString &key, const QVariant &v) const
{
    Q_ASSERT(settings());

    settings()->setCacheValue(key, v);
}

void MapSettings::removeGroup(const QString &group)
{
    const QString groupPrefix = group + '/';

    QVariantMap map = this->map();

    map.removeIf([&](QVariantMap::iterator it) { return it.key().startsWith(groupPrefix); });

    setMap(map);

    m_removedGroups.append(group);
}

void MapSettings::save()
{
    Q_ASSERT(settings());

    for (const QString &group : std::as_const(m_removedGroups)) {
        settings()->removeIniKey(group);
    }
    m_removedGroups.clear();

    for (const auto &[key, v] : QtCompat::keyValueRange(map())) {
        if (!isTransientKey(key)) {
            settings()->setIniValue(key, v);
        }
    }

    settings()->iniFlush();
}

void MapSettings::saveAndClear()
{
    save();
    clear();
}
