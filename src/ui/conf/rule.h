#ifndef RULE_H
#define RULE_H

#include <QDateTime>
#include <QObject>

struct RuleIdRange
{
    int minId;
    int maxId;
};

class Rule
{
public:
    enum RuleType : quint8 {
        AppRule = 0, // 1..64
        GlobalBeforeAppsRule, // 65..96
        GlobalAfterAppsRule, // 97..128
        PresetRule, // 129..255
        RuleTypeCount
    };

    bool isNameEqual(const Rule &o) const;
    bool isOptionsEqual(const Rule &o) const;

    static RuleType getRuleTypeById(int ruleId);
    static RuleIdRange getRuleIdRangeByType(RuleType ruleType);

public:
    bool enabled : 1 = true;
    bool blocked : 1 = false;
    bool exclusive : 1 = false;

    RuleType ruleType = AppRule;

    int ruleId = 0;

    quint32 acceptZones = 0;
    quint32 rejectZones = 0;

    quint32 presetRules = 0;

    QString ruleName;
    QString notes;
    QString ruleText;

    QDateTime modTime;
};

#endif // RULE_H
