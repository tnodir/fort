#include "inigraph.h"

#include <util/ini/mapsettings.h>

namespace {

struct GraphColorInfo
{
    const char *keys[2]; // light, dark
    QColor colors[2];
};

// Sync with GraphColorType
inline constexpr GraphColorInfo graphColorInfos[] = {
    { { "color", "darkColor" }, { QColor(255, 255, 255), QColor(56, 56, 56) } },
    { { "borderColor", "darkBorderColor" }, { QColor(108, 108, 108), QColor(141, 141, 141) } },
    { { "colorIn", "darkColorIn" }, { QColor(52, 196, 84), QColor(52, 196, 84) } },
    { { "colorOut", "darkColorOut" }, { QColor(235, 71, 63), QColor(235, 71, 63) } },
    { { "totalColor", "darkTotalColor" }, { QColor(245, 184, 0), QColor(245, 184, 0) } },
    { { "axisColor", "darkAxisColor" }, { QColor(108, 108, 108), QColor(141, 141, 141) } },
    { { "tickLabelColor", "darkTickLabelColor" }, { QColor(0, 0, 0), QColor(248, 248, 248) } },
    { { "labelColor", "darkLabelColor" }, { QColor(33, 33, 33), QColor(248, 248, 248) } },
    { { "gridColor", "darkGridColor" }, { QColor(141, 141, 141), QColor(108, 108, 108) } },
};

static_assert(std::size(graphColorInfos) == GraphColorCount,
        "Missing a GraphColorType in graphColorInfos[]");

struct GraphFontInfo
{
    const char *key;
    int size;
};

// Sync with GraphFontType
inline constexpr GraphFontInfo graphFontInfos[] = {
    { "tickLabelFont", 9 },
    { "speedLabelFont", 10 },
};

static_assert(
        std::size(graphFontInfos) == GraphFontCount, "Missing a GraphFontType in graphFontInfos[]");

}

IniGraph::IniGraph(MapSettings &ini, int graphId) :
    m_graphId(graphId), m_ini(ini), m_group(groupName(graphId))
{
}

QString IniGraph::groupName(int graphId)
{
    const QString name("graphWindow");

    return (graphId == 0) ? name : name + QString::number(graphId);
}

void IniGraph::remove()
{
    m_ini.removeGroup(m_group);
}

const char *IniGraph::colorKey(GraphColorType type, bool dark)
{
    return graphColorInfos[type].keys[dark];
}

QColor IniGraph::colorDefault(GraphColorType type, bool dark)
{
    return graphColorInfos[type].colors[dark];
}

QColor IniGraph::color(GraphColorType type, bool dark) const
{
    return valueColor(colorKey(type, dark), colorDefault(type, dark));
}

void IniGraph::setColor(GraphColorType type, bool dark, const QColor &v)
{
    setColorValue(colorKey(type, dark), v);
}

QFont IniGraph::fontDefault(GraphFontType type, const QFont &baseFont)
{
    QFont font = baseFont;
    font.setPointSize(graphFontInfos[type].size);
    font.setWeight(QFont::DemiBold);

    return font;
}

QFont IniGraph::font(GraphFontType type, const QFont &baseFont) const
{
    QFont font = fontDefault(type, baseFont);

    const QString text = value(graphFontInfos[type].key).toString();
    if (!text.isEmpty()) {
        font.fromString(text);
    }

    return font;
}

void IniGraph::setFont(GraphFontType type, const QFont &v, const QFont &baseFont)
{
    // The default font isn't saved to follow the base font's change
    const bool isDefault = (v == fontDefault(type, baseFont));

    setValue(graphFontInfos[type].key, isDefault ? QString() : v.toString());
}

QString IniGraph::key(const char *name) const
{
    return m_group + '/' + QLatin1String(name);
}

QVariant IniGraph::value(const char *name) const
{
    return m_ini.value(key(name));
}

void IniGraph::setValue(const char *name, const QVariant &v)
{
    m_ini.setValue(key(name), v);
}

int IniGraph::valueInt(const char *name, int defaultValue) const
{
    return m_ini.valueInt(key(name), defaultValue);
}

bool IniGraph::valueBool(const char *name, bool defaultValue) const
{
    return m_ini.valueBool(key(name), defaultValue);
}

QColor IniGraph::valueColor(const char *name, const QColor &defaultValue) const
{
    return m_ini.valueColor(key(name), defaultValue);
}

void IniGraph::setColorValue(const char *name, const QColor &v)
{
    m_ini.setColor(key(name), v);
}
