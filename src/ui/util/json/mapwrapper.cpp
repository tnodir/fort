#include "mapwrapper.h"

MapWrapper::MapWrapper(const QVariant &var) : m_map(var.toMap()) { }

MapWrapper::MapWrapper(const MapWrapper &o) : MapWrapper(o.map()) { }

void MapWrapper::clear()
{
    m_map.clear();
}

int MapWrapper::valueInt(const QString &key, int defaultValue) const
{
    return value(key, defaultValue).toInt();
}

bool MapWrapper::valueBool(const QString &key, bool defaultValue) const
{
    return value(key, defaultValue).toBool();
}

uint MapWrapper::valueUInt(const QString &key, int defaultValue) const
{
    return value(key, defaultValue).toUInt();
}

qreal MapWrapper::valueReal(const QString &key, qreal defaultValue) const
{
    return value(key, defaultValue).toReal();
}

QString MapWrapper::valueText(const QString &key, const QString &defaultValue) const
{
    return value(key, defaultValue).toString();
}

QStringList MapWrapper::valueList(const QString &key) const
{
    return value(key).toStringList();
}

QVariantMap MapWrapper::valueMap(const QString &key) const
{
    return value(key).toMap();
}

QByteArray MapWrapper::valueByteArray(const QString &key) const
{
    return value(key).toByteArray();
}

QColor MapWrapper::valueColor(const QString &key, const QColor &defaultValue) const
{
    const QString text = valueText(key);
    if (text.isEmpty())
        return defaultValue;

    if (text.at(0).isDigit())
        return QColor::fromRgba(text.toUInt());

    return { text };
}

void MapWrapper::setColor(const QString &key, const QColor &v)
{
    setValue(key, v.name());
}

QVariant MapWrapper::value(const QString &key, const QVariant &defaultValue) const
{
    return map().value(key, defaultValue);
}

void MapWrapper::setValue(const QString &key, const QVariant &v, const QVariant & /*defaultValue*/)
{
    m_map.insert(key, v);
}

bool MapWrapper::contains(const QString &key) const
{
    return map().contains(key);
}
