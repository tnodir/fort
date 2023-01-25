#ifndef RULE_H
#define RULE_H

#include <QObject>

class Rule
{
public:
    enum RuleFlag {
        FlagNone = 0,
        FlagInbound = (1 << 0),
        FlagEqualPorts = (1 << 1),
    };

    bool enabled = true;
    bool block = false;
    bool report = false;
    bool log = false;

    quint16 ruleFlags = 0;
    quint16 ipProto = 0;

    int ruleId = 0;

    QString name;
    QString description;

    QString localPortText;
    QString remotePortText;
    QString localIpText;
    QString remoteIpText;

    QString extra;
};

#endif // RULE_H
