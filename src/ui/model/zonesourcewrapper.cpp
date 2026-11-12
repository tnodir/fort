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

bool ZoneSourceWrapper::subdomains() const
{
    return valueBool("subdomains");
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
    const int sourceId = id();

    return sourceId == SourceText || sourceId == SourceHostsText;
}

int ZoneSourceWrapper::idByCode(const QString &code)
{
    static const QHash<QString, int> map = {
        { "text", SourceText },
        { "file", SourceFile },
        { "host_text", SourceHostsText },
        { "host_file", SourceHostsFile },
        { "domain_file", SourceDomainsFile },
        { "domain_hosts_file", SourceDomainsHostsFile },
        { "winspyblock", SourceWinSpyBlock },
        { "firehol_level1", SourceFireHol_Level1 },
        { "tasix", SourceTasix },
    };

    return map.value(code, SourceText);
}
