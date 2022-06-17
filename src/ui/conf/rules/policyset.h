#ifndef POLICYSET_H
#define POLICYSET_H

#include <QObject>

class Policy;
class Rule;

using PolicyList = QList<Policy *>;
using RuleList = QList<Rule *>;

class PolicySet
{
public:
    Policy *m_policy = nullptr;
    RuleList m_rules;
    PolicyList m_policies;
};

#endif // POLICYSET_H
