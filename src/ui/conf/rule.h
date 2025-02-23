#ifndef RULE_H
#define RULE_H

#include <QDateTime>
#include <QObject>
#include <QVector>

using RuleSetList = QVector<quint16>;

class Rule
{
public:
    enum RuleType : qint8 {
        RuleNone = -1,
        AppRule = 0,
        GlobalBeforeAppsRule,
        GlobalAfterAppsRule,
        PresetRule,
        RuleTypeCount
    };

    enum TerminateActionType : qint8 {
        TerminateAllow = 0,
        TerminateBlock,
    };

    bool isNameEqual(const Rule &o) const;
    bool isOptionsEqual(const Rule &o) const;
    bool isFlagsEqual(const Rule &o) const;

    int terminateActionType() const;
    void setTerminateActionType(qint8 v);

public:
    bool enabled : 1 = true;
    bool blocked : 1 = false;
    bool exclusive : 1 = false;
    bool terminate : 1 = false;
    bool terminateBlocked : 1 = true;
    bool ruleSetEdited : 1 = false; // transient

    RuleType ruleType = AppRule;

    quint16 ruleId = 0;

    quint32 acceptZones = 0;
    quint32 rejectZones = 0;

    QString ruleName;
    QString notes;
    QString ruleText;

    QDateTime modTime;

    RuleSetList ruleSet;
};

#endif // RULE_H
