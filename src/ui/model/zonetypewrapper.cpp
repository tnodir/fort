#include "zonetypewrapper.h"

ZoneTypeWrapper::ZoneTypeWrapper(const QVariant &var) : MapWrapper(var) { }

int ZoneTypeWrapper::id() const
{
    return valueInt("id");
}

void ZoneTypeWrapper::setId(int id)
{
    setValue("id", id);
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

int ZoneTypeWrapper::idByCode(const QString &code)
{
    static const QHash<QString, int> map = {
        { "gen", TypeGeneric },
        { "bgp", TypeBgp },
    };

    return map.value(code, TypeGeneric);
}
