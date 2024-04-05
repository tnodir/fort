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
const int listCount = sizeof(list) / sizeof(list[0]);

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

QString IniUser::hotKeyValue(const QString &key) const
{
    const auto &defaultValue = HotKey::defaultValue(key.toLatin1());

    return valueText("hotKey/" + key, defaultValue);
}

void IniUser::setHotKeyValue(const QString &key, const QString &v)
{
    setValue("hotKey/" + key, v);
}

void IniUser::saveDefaultIni()
{
    setLanguage(defaultLanguage());

    saveAndClear();
}
