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

using walkRulesCallback = bool(const Rule &rule);

class ConfRulesWalker
{
public:
    virtual bool walkRules(ruleset_map_t &ruleSetMap, ruleid_arr_t &ruleSetIds, int &maxRuleId,
            const std::function<walkRulesCallback> &func) const = 0;
};

#endif // CONFRULESWALKER_H
