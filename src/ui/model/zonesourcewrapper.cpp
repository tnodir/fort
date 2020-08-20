#include "zonesourcewrapper.h"

ZoneSourceWrapper::ZoneSourceWrapper(const QVariant &var) : MapWrapper(var) { }

int ZoneSourceWrapper::index() const
{
    return valueInt("index");
}

void ZoneSourceWrapper::setIndex(int index)
{
    setValueInt("index", index);
}

QString ZoneSourceWrapper::code() const
{
    return valueText("code");
}

QString ZoneSourceWrapper::title() const
{
    return valueText("title");
}

QString ZoneSourceWrapper::zoneType() const
{
    return valueText("zoneType");
}

QString ZoneSourceWrapper::url() const
{
    return valueText("url");
}

QString ZoneSourceWrapper::formData() const
{
    return valueText("formData");
}
