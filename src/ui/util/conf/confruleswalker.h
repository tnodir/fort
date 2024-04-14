#ifndef CONFRULESWALKER_H
#define CONFRULESWALKER_H

#include <QHash>
#include <QObject>
#include <QVector>

#include <functional>

#include <conf/rule.h>

struct RuleSetIndex
{
    quint32 index : 24;
    quint32 count : 8;
};

using ruleset_map_t = QHash<quint16, RuleSetIndex>;
using ruleid_arr_t = QVector<quint16>;

using walkRulesCallback = bool(Rule &rule);

class ConfRulesWalker
{
public:
    virtual bool walkRules(ruleset_map_t &ruleSetMap, ruleid_arr_t &ruleIds, int &maxRuleId,
            const std::function<walkRulesCallback> &func) const = 0;
};

#endif // CONFRULESWALKER_H
