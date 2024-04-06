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
    blockTraffic,
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
    static QHash<const char *, const char *> g_defaultValuesMap = {
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
