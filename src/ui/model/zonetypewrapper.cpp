#include "zonetypewrapper.h"

ZoneTypeWrapper::ZoneTypeWrapper(const QVariant &var) : MapWrapper(var) { }

int ZoneTypeWrapper::index() const
{
    return valueInt("index");
}

void ZoneTypeWrapper::setIndex(int index)
{
    setValue("index", index);
}

QString ZoneTypeWrapper::code() const
{
    return valueText("code");
}

QString ZoneTypeWrapper::title() const
{
    return valueText("title");
}

QString ZoneTypeWrapper::description() const
{
    return valueText("description");
}

bool ZoneTypeWrapper::sort() const
{
    return valueBool("sort");
}

QString ZoneTypeWrapper::pattern() const
{
    return valueText("pattern");
}

int ZoneTypeWrapper::emptyNetMask() const
{
    return valueInt("emptyNetMask");
}
