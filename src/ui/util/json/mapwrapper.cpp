#include "mapwrapper.h"

MapWrapper::MapWrapper(const QVariant &var) : m_map(var.toMap()) { }

int MapWrapper::valueInt(const QString &key) const
{
    return value(key).toInt();
}

bool MapWrapper::valueBool(const QString &key) const
{
    return value(key).toBool();
}

QString MapWrapper::valueText(const QString &key) const
{
    return value(key).toString();
}

QVariant MapWrapper::value(const QString &key) const
{
    return map().value(key);
}

void MapWrapper::setValueInt(const QString &key, int v)
{
    setValue(key, v);
}

void MapWrapper::setValue(const QString &key, const QVariant &v)
{
    m_map[key] = v;
}
