#ifndef RULE_H
#define RULE_H

#include <QDateTime>
#include <QObject>

class Rule
{
public:
    bool isNameEqual(const Rule &o) const;
    bool isOptionsEqual(const Rule &o) const;

public:
    bool enabled = true;
    bool blocked = false;
    bool exclusive = false;

    int ruleId = 0;

    quint32 acceptZones = 0;
    quint32 rejectZones = 0;

    QString ruleName;
    QString notes;
    QString ruleText;

    QDateTime modTime;
};

#endif // RULE_H
