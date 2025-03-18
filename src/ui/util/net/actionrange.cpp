#include "actionrange.h"

#include <common/fortconf.h>

#include <util/conf/confdata.h>

ActionRange::ActionRange(QObject *parent) : TextRange(parent) { }

bool ActionRange::isEmpty() const
{
    return m_actionTypeId == 0;
}

void ActionRange::clear()
{
    TextRange::clear();

    m_actionTypeId = 0;
}

void ActionRange::toList(QStringList &list) const
{
    if (m_actionTypeId == FORT_RULE_FILTER_ACTION_ALLOW) {
        list << "ALLOW";
    }
    if (m_actionTypeId == FORT_RULE_FILTER_ACTION_BLOCK) {
        list << "BLOCK";
    }
}

TextRange::ParseError ActionRange::parseText(const QString &text)
{
    if (text == "ALLOW") {
        m_actionTypeId = FORT_RULE_FILTER_ACTION_ALLOW;
    } else if (text == "BLOCK") {
        m_actionTypeId = FORT_RULE_FILTER_ACTION_BLOCK;
    } else {
        return ErrorBadText;
    }

    return ErrorOk;
}

void ActionRange::write(ConfData &confData) const
{
    confData.writeActionRange(*this);
}
