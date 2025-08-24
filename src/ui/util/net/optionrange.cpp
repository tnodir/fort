#include "optionrange.h"

#include <common/fortconf.h>

#include <util/conf/confdata.h>

OptionRange::OptionRange(QObject *parent) : TextRange(parent) { }

bool OptionRange::isEmpty() const
{
    return m_optionTypeIds == 0;
}

void OptionRange::clear()
{
    TextRange::clear();

    m_optionTypeIds = 0;
}

void OptionRange::toList(QStringList &list) const
{
    if ((m_optionTypeIds & FORT_RULE_FILTER_OPTION_LOG) != 0) {
        list << "LOG";
    }
    if ((m_optionTypeIds & FORT_RULE_FILTER_OPTION_ALERT) != 0) {
        list << "ALERT";
    }
}

TextRange::ParseError OptionRange::parseText(const QString &text)
{
    if (text == "LOG") {
        m_optionTypeIds |= FORT_RULE_FILTER_OPTION_LOG;
    } else if (text == "ALERT") {
        m_optionTypeIds |= FORT_RULE_FILTER_OPTION_ALERT;
    } else {
        return ErrorBadText;
    }

    return ErrorOk;
}

void OptionRange::write(ConfData &confData) const
{
    confData.writeOptionRange(*this);
}
