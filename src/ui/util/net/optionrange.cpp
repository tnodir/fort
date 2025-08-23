#include "optionrange.h"

#include <common/fortconf.h>

#include <util/conf/confdata.h>

OptionRange::OptionRange(QObject *parent) : TextRange(parent) { }

bool OptionRange::isEmpty() const
{
    return m_optionTypeId == 0;
}

void OptionRange::clear()
{
    TextRange::clear();

    m_optionTypeId = 0;
}

void OptionRange::toList(QStringList &list) const
{
    if (m_optionTypeId == FORT_RULE_FILTER_OPTION_LOG) {
        list << "LOG";
    }
    if (m_optionTypeId == FORT_RULE_FILTER_OPTION_ALERT) {
        list << "ALERT";
    }
}

TextRange::ParseError OptionRange::parseText(const QString &text)
{
    if (text == "LOG") {
        m_optionTypeId = FORT_RULE_FILTER_OPTION_LOG;
    } else if (text == "ALERT") {
        m_optionTypeId = FORT_RULE_FILTER_OPTION_ALERT;
    } else {
        return ErrorBadText;
    }

    return ErrorOk;
}

void OptionRange::write(ConfData &confData) const
{
    confData.writeOptionRange(*this);
}
