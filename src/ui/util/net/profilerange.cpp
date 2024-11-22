#include "profilerange.h"

#include <common/fortconf.h>

#include <util/conf/confdata.h>

ProfileRange::ProfileRange(QObject *parent) : TextRange(parent) { }

bool ProfileRange::isEmpty() const
{
    return m_profileId == 0;
}

void ProfileRange::clear()
{
    TextRange::clear();

    m_profileId = 0;
}

void ProfileRange::toList(QStringList &list) const
{
    if (m_profileId == FORT_RULE_FILTER_PROFILE_PUBLIC) {
        list << "PUBLIC";
    }
    if (m_profileId == FORT_RULE_FILTER_PROFILE_PRIVATE) {
        list << "PRIVATE";
    }
    if (m_profileId == FORT_RULE_FILTER_PROFILE_DOMAIN) {
        list << "DOMAIN";
    }
}

TextRange::ParseError ProfileRange::parseText(const QString &text)
{
    if (text == "PUBLIC") {
        m_profileId = FORT_RULE_FILTER_PROFILE_PUBLIC;
    } else if (text == "PRIVATE") {
        m_profileId = FORT_RULE_FILTER_PROFILE_PRIVATE;
    } else if (text == "DOMAIN") {
        m_profileId = FORT_RULE_FILTER_PROFILE_DOMAIN;
    } else {
        return ErrorBadText;
    }

    return ErrorOk;
}

void ProfileRange::write(ConfData &confData) const
{
    confData.writeProfileRange(*this);
}
