#include "iniuser.h"

#include <QHash>

#include <util/bitutil.h>

#include "inigraph.h"

namespace HotKey {

const char *const list[] = {
    main,
    programs,
    options,
    rules,
    zones,
    groups,
    speedLimits,
    timePeriods,
    services,
    traffic,
    activity,
    connections,
    filterSim,
    graph,
    filter,
    snoozeAlerts,
    blockTrafficOff,
    blockInetTraffic,
    blockLanTraffic,
    blockInetLanTraffic,
    blockTraffic,
    filterModeAutoLearn,
    filterModeAskToConnect,
    filterModeBlock,
    filterModeAllow,
    filterModeIgnore,
    appGroupModifier,
    ruleModifier,
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

quint8 IniUser::graphWindowIdsMask() const
{
    quint8 mask = 1; // the main graph's

    const QStringList list = valueList("graphsWindow/graphIds");

    for (const QString &s : list) {
        const int graphId = s.toInt();

        if (graphId > 0 && graphId < IniGraph::maxCount) {
            mask |= quint8(1 << graphId);
        }
    }

    return mask;
}

void IniUser::setGraphWindowIdsMask(quint8 v)
{
    QStringList list;

    for (int graphId = 1; graphId < IniGraph::maxCount; ++graphId) {
        if ((v & (1 << graphId)) != 0) {
            list.append(QString::number(graphId));
        }
    }

    setValue("graphsWindow/graphIds", list);
}

QVector<int> IniUser::graphWindowIds() const
{
    QVector<int> graphIds;

    const quint8 mask = graphWindowIdsMask();

    for (int graphId = 0; graphId < IniGraph::maxCount; ++graphId) {
        if ((mask & (1 << graphId)) != 0) {
            graphIds.append(graphId);
        }
    }

    return graphIds;
}

int IniUser::newGraphWindowId() const
{
    const int graphId = BitUtil::firstZeroBit(graphWindowIdsMask());

    return (graphId < IniGraph::maxCount) ? graphId : -1;
}

void IniUser::addGraphWindowId(int graphId)
{
    setGraphWindowIdsMask(graphWindowIdsMask() | (1 << graphId));
}

void IniUser::removeGraphWindowId(int graphId)
{
    IniGraph(*this, graphId).remove();

    setGraphWindowIdsMask(graphWindowIdsMask() & ~(1 << graphId));
}

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

    addDefaultTaskbarGraph();

    saveAndClear();
}

void IniUser::addDefaultTaskbarGraph()
{
    const int graphId = newGraphWindowId();

    IniGraph ini(*this, graphId);
    ini.setName("Taskbar");
    ini.setShowInTaskbar(true);
    ini.setGraphType(GraphTypeLine);
    ini.setYAxisLabels(false);
    ini.setShowSpeed(true);
    ini.setSpeedPosition(GraphSpeedLeft);
    ini.setSpeedInside(false);
    ini.setColor(GraphColorBg, /*dark=*/false, Qt::transparent);
    ini.setColor(GraphColorBg, /*dark=*/true, Qt::transparent);

    addGraphWindowId(graphId);
}

int IniUser::colorSchemeByName(const QString &theme)
{
    if (theme == QLatin1String("light"))
        return ColorSchemeLight;

    if (theme == QLatin1String("dark"))
        return ColorSchemeDark;

    return ColorSchemeSystem;
}

QString IniUser::colorSchemeName(int colorScheme)
{
    switch (colorScheme) {
    case ColorSchemeLight:
        return QLatin1String("light");

    case ColorSchemeDark:
        return QLatin1String("dark");
    }

    return {};
}
