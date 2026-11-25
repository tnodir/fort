#include "inigraph.h"

#include <util/ini/mapsettings.h>

IniGraph::IniGraph(MapSettings &ini, int graphId) :
    m_graphId(graphId), m_ini(ini), m_group(groupName(graphId))
{
}

QString IniGraph::groupName(int graphId)
{
    const QString name("graphWindow");

    return (graphId == 0) ? name : name + QString::number(graphId);
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
