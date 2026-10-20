#ifndef RULE_H
#define RULE_H

#include <QDateTime>
#include <QObject>
#include <QVector>

#include <common/common_types.h>

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

    static constexpr quint8 ruleTypeBit(RuleType ruleType) { return quint8(1 << ruleType); }

    bool isNameEqual(const Rule &o) const;
    bool isOptionsEqual(const Rule &o) const;
    bool isFlagsEqual(const Rule &o) const;
    bool isTerminateFlagsEqual(const Rule &o) const;
    bool isLogFlagsEqual(const Rule &o) const;
    bool isZonesEqual(const Rule &o) const;
    bool isPeriodEqual(const Rule &o) const;

    // By the Terminating flags: Allow, Block, Ask or Drop of FORT_RULE_FILTER_ACTION_*
    quint8 terminateActionTypeId() const;
    void setTerminateActionTypeId(quint8 actionTypeId);

public:
    bool enabled : 1 = true;
    bool trayMenu : 1 = false;
    bool blocked : 1 = false;
    bool ask : 1 = false; // blocked till the user's decision
    bool exclusive : 1 = false;
    bool inlineZones : 1 = false;
    bool terminate : 1 = false;
    bool terminateBlocked : 1 = true;
    bool terminateDrop : 1 = false;
    bool terminateAsk : 1 = false;
    bool terminateAlert : 1 = false;
    bool logAllowedConn : 1 = true;
    bool logBlockedConn : 1 = true;
    bool periodEnabled : 1 = true;
    bool ruleSetEdited : 1 = false; // transient

    RuleType ruleType = AppRule;

    quint8 periodId = 0;

    quint16 ruleId = 0;

    FORT_CONF_RULE_ZONES zones = {};

    QString ruleName;
    QString notes;
    QString ruleText;

    QDateTime modTime;

    RuleSetList ruleSet;
};

#endif // RULE_H
