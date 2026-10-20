#include "actionrange.h"

#include <bit>

#include <util/conf/confdata.h>

namespace {

struct ActionType
{
    quint8 id;
    const char *name; // also the translated text's source
    const char *iconPath;
};

// By their ids' bits: Allow, Block, Ask, Drop
inline constexpr ActionType g_actionTypes[] = {
    {
            FORT_RULE_FILTER_ACTION_ALLOW,
            QT_TRANSLATE_NOOP("ActionRange", "Allow"),
            ":/icons/accept.png",
    },
    {
            FORT_RULE_FILTER_ACTION_BLOCK,
            QT_TRANSLATE_NOOP("ActionRange", "Block"),
            ":/icons/deny.png",
    },
    {
            FORT_RULE_FILTER_ACTION_ASK,
            QT_TRANSLATE_NOOP("ActionRange", "Ask"),
            ":/icons/help.png",
    },
    {
            FORT_RULE_FILTER_ACTION_DROP,
            QT_TRANSLATE_NOOP("ActionRange", "Drop"),
            ":/icons/cross.png",
    },
};

const ActionType *findActionType(quint8 actionTypeId)
{
    const int index = ActionRange::actionTypeIndex(actionTypeId);
    if (index < 0)
        return nullptr;

    const ActionType &actionType = g_actionTypes[index];

    Q_ASSERT(actionType.id == actionTypeId);

    return &actionType;
}

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
    const ActionType *actionType = findActionType(actionTypeId);

    return actionType ? actionType->name : QString();
}

quint8 ActionRange::actionTypeIdByName(const QString &name)
{
    for (const auto &actionType : g_actionTypes) {
        if (name.compare(QLatin1String(actionType.name), Qt::CaseInsensitive) == 0) {
            return actionType.id;
        }
    }

    return 0;
}

QString ActionRange::actionTypeText(quint8 actionTypeId)
{
    const ActionType *actionType = findActionType(actionTypeId);

    return actionType ? tr(actionType->name) : QString();
}

QString ActionRange::actionTypeIconPath(quint8 actionTypeId)
{
    const ActionType *actionType = findActionType(actionTypeId);

    return actionType ? actionType->iconPath : QString();
}

quint8 ActionRange::actionTypeIdAt(int index)
{
    if (index < 0 || index >= std::ssize(g_actionTypes))
        return 0;

    return g_actionTypes[index].id;
}

int ActionRange::actionTypeIndex(quint8 actionTypeId)
{
    const int index = std::countr_zero(actionTypeId);

    return (index < std::ssize(g_actionTypes)) ? index : -1;
}

QStringList ActionRange::actionTypeTexts()
{
    QStringList list;
    for (const auto &actionType : g_actionTypes) {
        list << tr(actionType.name);
    }
    return list;
}

QStringList ActionRange::actionTypeIconPaths()
{
    QStringList list;
    for (const auto &actionType : g_actionTypes) {
        list << actionType.iconPath;
    }
    return list;
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
