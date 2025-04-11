#include "arearange.h"

#include <util/conf/confdata.h>

AreaRange::AreaRange(QObject *parent) : TextRange(parent) { }

bool AreaRange::isEmpty() const
{
    return !(isLocalhost() || isLan() || isInet());
}

void AreaRange::clear()
{
    TextRange::clear();

    m_isLocalhost = false;
    m_isLan = false;
    m_isInet = false;
}

void AreaRange::toList(QStringList &list) const
{
    if (isLocalhost()) {
        list << "LOCALHOST";
    }
    if (isLan()) {
        list << "LAN";
    }
    if (isInet()) {
        list << "INET";
    }
}

TextRange::ParseError AreaRange::parseText(const QString &text)
{
    if (text == "LOCALHOST") {
        m_isLocalhost = true;
    } else if (text == "LAN") {
        m_isLan = true;
    } else if (text == "INET" || text == "INTERNET") {
        m_isInet = true;
    } else {
        return ErrorBadText;
    }

    return ErrorOk;
}

void AreaRange::write(ConfData &confData) const
{
    confData.writeAreaRange(*this);
}
