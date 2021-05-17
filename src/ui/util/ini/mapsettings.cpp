#include "mapsettings.h"

#include "settings.h"

MapSettings::MapSettings(Settings *settings) : m_settings(settings) { }

MapSettings::MapSettings(const MapSettings &o) : MapWrapper(o.map()), m_settings(o.settings()) { }

void MapSettings::save() const
{
    Q_ASSERT(settings());

    auto it = map().constBegin();
    const auto end = map().constEnd();

    for (; it != end; ++it) {
        const QString &key = it.key();
        if (!isTransientKey(key)) {
            settings()->setIniValue(key, it.value());
        }
    }

    settings()->iniFlush();
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

bool MapSettings::isTransientKey(const QString &key)
{
    return key.endsWith('_');
}
