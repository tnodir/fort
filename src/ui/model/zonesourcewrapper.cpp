#include "zonesourcewrapper.h"

#include <QHash>

ZoneSourceWrapper::ZoneSourceWrapper(const QVariant &var) : MapWrapper(var) { }

int ZoneSourceWrapper::id() const
{
    return valueInt("id");
}

void ZoneSourceWrapper::setId(int id)
{
    setValue("id", id);
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

bool ZoneSourceWrapper::isTextInline() const
{
    return code() == defaultSourceCode();
}

QString ZoneSourceWrapper::defaultSourceCode()
{
    return "text";
}
