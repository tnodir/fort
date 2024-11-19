#ifndef CONFRULESWALKER_H
#define CONFRULESWALKER_H

#include <QHash>
#include <QObject>
#include <QVector>

#include <functional>

class Rule;

struct RuleSetInfo
{
    quint32 index : 24;
    quint32 count : 8;
};

using ruleset_map_t = QHash<quint16, RuleSetInfo>;
using ruleid_arr_t = QVector<quint16>;

struct WalkRulesArgs
{
    ruleset_map_t ruleSetMap;
    ruleid_arr_t ruleSetIds;

    quint16 maxRuleId = 0;
    quint16 globPreRuleId = 0;
    quint16 globPostRuleId = 0;
};

using walkRulesCallback = bool(const Rule &rule);

class ConfRulesWalker
{
public:
    virtual bool walkRules(
            WalkRulesArgs &wra, const std::function<walkRulesCallback> &func) const = 0;
};

#endif // CONFRULESWALKER_H
