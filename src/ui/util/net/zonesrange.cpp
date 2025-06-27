#include "zonesrange.h"

#include <util/conf/confdata.h>

ZonesRange::ZonesRange(QObject *parent) : TextRange(parent) { }

bool ZonesRange::isEmpty() const
{
    return !(isResult() || isAccepted() || isRejected());
}

void ZonesRange::clear()
{
    TextRange::clear();

    m_isResult = false;
    m_isAccepted = false;
    m_isRejected = false;
}

void ZonesRange::toList(QStringList &list) const
{
    if (isResult()) {
        list << "RESULT";
    }
    if (isAccepted()) {
        list << "ACCEPTED";
    }
    if (isRejected()) {
        list << "REJECTED";
    }
}

TextRange::ParseError ZonesRange::parseText(const QString &text)
{
    if (text == "RESULT") {
        m_isResult = true;
    } else if (text == "ACCEPTED") {
        m_isAccepted = true;
    } else if (text == "REJECTED") {
        m_isRejected = true;
    } else {
        return ErrorBadText;
    }

    return ErrorOk;
}

void ZonesRange::write(ConfData &confData) const
{
    confData.writeZonesRange(*this);
}
