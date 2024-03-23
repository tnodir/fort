#include "rule.h"

bool Rule::isNameEqual(const Rule &o) const
{
    return ruleName == o.ruleName;
}

bool Rule::isOptionsEqual(const Rule &o) const
{
    return enabled == o.enabled && blocked == o.blocked && exclusive == o.exclusive
            && acceptZones == o.acceptZones && rejectZones == o.rejectZones
            && presetRules == o.presetRules && ruleName == o.ruleName && notes == o.notes
            && ruleText == o.ruleText;
}

Rule::RuleType Rule::getRuleTypeById(int ruleId)
{
    for (int i = 0; i < RuleTypeCount; ++i) {
        const auto ruleType = RuleType(i);
        const auto range = getRuleIdRangeByType(ruleType);

        if (ruleId >= range.minId && ruleId <= range.maxId)
            return ruleType;
    }

    Q_UNREACHABLE();
    return AppRule;
}

RuleIdRange Rule::getRuleIdRangeByType(RuleType ruleType)
{
    static const RuleIdRange ruleIdRanges[] = { { 1, 64 }, { 65, 96 }, { 97, 128 }, { 129, 255 } };

    Q_ASSERT(ruleType >= 0 && ruleType < RuleTypeCount);

    return ruleIdRanges[ruleType];
}
