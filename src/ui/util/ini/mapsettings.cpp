#include "mapsettings.h"

#include "settings.h"

MapSettings::MapSettings(Settings *settings) : m_settings(settings) { }

MapSettings::MapSettings(const MapSettings &o) : MapWrapper(o.map()), m_settings(o.settings()) { }

void MapSettings::save() const
{
    Q_ASSERT(settings());

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    for (const auto &[key, v] : map().asKeyValueRange()) {
#else
    auto it = map().constBegin();
    const auto end = map().constEnd();

    for (; it != end; ++it) {
        const QString &key = it.key();
        const QVariant &v = it.value();
#endif

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
