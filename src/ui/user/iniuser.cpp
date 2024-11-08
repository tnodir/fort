#include "iniuser.h"

#include <QHash>

namespace HotKey {

const char *const list[] = {
    home,
    programs,
    options,
    rules,
    zones,
    statistics,
    graph,
    filter,
    blockTrafficOff,
    blockTraffic,
    blockLanTraffic,
    blockInetTraffic,
    filterModeAutoLearn,
    filterModeAskToConnect,
    filterModeBlock,
    filterModeAllow,
    filterModeIgnore,
    appGroupModifier,
    quit,
};
const int listCount = int(std::size(list));

const char *const defaultValue(const char *key)
{
    static const QHash<const char *, const char *> g_defaultValuesMap = {
        { filter, Default::filter },
        { appGroupModifier, Default::appGroupModifier },
    };

    return g_defaultValuesMap.value(key, nullptr);
}

}

IniUser::IniUser(Settings *settings) : MapSettings(settings) { }

QString IniUser::hotKeyValue(const char *key) const
{
    const auto &defaultValue = HotKey::defaultValue(key);

    return valueText("hotKey/" + QLatin1String(key), defaultValue);
}

void IniUser::setHotKeyValue(const char *key, const QString &v)
{
    setValue("hotKey/" + QLatin1String(key), v);
}

void IniUser::saveDefaultIni()
{
    setLanguage(defaultLanguage());

    saveAndClear();
}

int IniUser::colorSchemeByName(const QString &theme)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    if (theme == QLatin1String("light"))
        return int(Qt::ColorScheme::Light);

    if (theme == QLatin1String("dark"))
        return int(Qt::ColorScheme::Dark);
#else
    Q_UNUSED(theme);
#endif

    return 0;
}

QString IniUser::colorSchemeName(int colorScheme)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    switch (Qt::ColorScheme(colorScheme)) {
    case Qt::ColorScheme::Light:
        return QLatin1String("light");

    case Qt::ColorScheme::Dark:
        return QLatin1String("dark");
    }
#else
    Q_UNUSED(colorScheme);
#endif

    return {};
}
