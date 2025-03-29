#include "ipverrange.h"

#include <util/conf/confdata.h>

IpVerRange::IpVerRange(QObject *parent) : TextRange(parent) { }

bool IpVerRange::isEmpty() const
{
    return !(isV4() || isV6());
}

void IpVerRange::clear()
{
    TextRange::clear();

    m_isV4 = false;
    m_isV6 = false;
}

void IpVerRange::toList(QStringList &list) const
{
    if (isV4()) {
        list << "4";
    }
    if (isV6()) {
        list << "6";
    }
}

TextRange::ParseError IpVerRange::parseText(const QString &text)
{
    if (text == "4") {
        m_isV4 = true;
    } else if (text == "6") {
        m_isV6 = true;
    } else {
        return ErrorBadText;
    }

    return ErrorOk;
}

void IpVerRange::write(ConfData &confData) const
{
    confData.writeIpVerRange(*this);
}
