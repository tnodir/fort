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
