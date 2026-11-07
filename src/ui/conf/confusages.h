#ifndef CONFUSAGES_H
#define CONFUSAGES_H

#include <QList>
#include <QString>

#include <conf/rule.h>

struct ConfUsage
{
    qint64 id = 0;
    QString name;
};

using ConfUsageList = QList<ConfUsage>;

// The conf's items referencing a Rule or a Zone
struct ConfUsages
{
    enum Category : qint8 {
        CategoryAddressLists = 0, // id: FirewallConf::AddressList
        CategoryApps,
        CategoryGroups,
        CategoryRules, // + Rule::RuleType
        CategoryCount = CategoryRules + Rule::RuleTypeCount,
    };

    ConfUsageList lists[CategoryCount];
};

#endif // CONFUSAGES_H
