#include "actionrange.h"

#include <common/fortconf.h>

#include <util/conf/confdata.h>

namespace {

struct ActionType
{
    quint8 id;
    const char *name;
};

inline constexpr ActionType actionTypes[] = {
    { FORT_RULE_FILTER_ACTION_ALLOW, "Allow" },
    { FORT_RULE_FILTER_ACTION_BLOCK, "Block" },
    { FORT_RULE_FILTER_ACTION_ASK, "Ask" },
    { FORT_RULE_FILTER_ACTION_DROP, "Drop" },
};

}

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
    const QString name = actionTypeName(m_actionTypeId);

    if (!name.isEmpty()) {
        list << name.toUpper();
    }
}

QString ActionRange::actionTypeName(quint8 actionTypeId)
{
    for (const auto &actionType : actionTypes) {
        if (actionType.id == actionTypeId) {
            return actionType.name;
        }
    }

    return {};
}

quint8 ActionRange::actionTypeIdByName(const QString &name)
{
    for (const auto &actionType : actionTypes) {
        if (name.compare(QLatin1String(actionType.name), Qt::CaseInsensitive) == 0) {
            return actionType.id;
        }
    }

    return 0;
}

TextRange::ParseError ActionRange::parseText(const QString &text)
{
    const quint8 actionTypeId = actionTypeIdByName(text);
    if (actionTypeId == 0)
        return ErrorBadText;

    m_actionTypeId = actionTypeId;

    return ErrorOk;
}

void ActionRange::write(ConfData &confData) const
{
    confData.writeActionRange(*this);
}
