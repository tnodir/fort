#pragma once

#include <conf/app.h>
#include <conf/group.h>
#include <conf/rule.h>
#include <util/conf/confappswalker.h>
#include <util/conf/confgroupswalker.h>
#include <util/conf/confruleswalker.h>

class TestApps : public ConfAppsWalker
{
public:
    explicit TestApps(const QList<App> &apps) : m_apps(apps) { }

    bool walkApps(const std::function<walkAppsCallback> &func) const override
    {
        for (App app : m_apps) {
            if (!func(app))
                return false;
        }
        return true;
    }

private:
    QList<App> m_apps;
};

class TestRuleList : public ConfRulesWalker
{
public:
    explicit TestRuleList(
            const QList<Rule> &rules, quint16 globPreRuleId = 0, quint16 globPostRuleId = 0) :
        m_globPreRuleId(globPreRuleId), m_globPostRuleId(globPostRuleId), m_rules(rules)
    {
    }

    bool walkRules(WalkRulesArgs &wra, const std::function<walkRulesCallback> &func) const override
    {
        for (const Rule &rule : m_rules) {
            wra.maxRuleId = qMax(wra.maxRuleId, rule.ruleId);
        }

        wra.globPreRuleId = m_globPreRuleId;
        wra.globPostRuleId = m_globPostRuleId;

        for (const Rule &rule : m_rules) {
            if (!func(rule))
                return false;
        }
        return true;
    }

private:
    quint16 m_globPreRuleId = 0;
    quint16 m_globPostRuleId = 0;

    QList<Rule> m_rules;
};

class TestGroupList : public ConfGroupsWalker
{
public:
    explicit TestGroupList(const QList<Group> &groups) : m_groups(groups) { }

    bool walkGroups(const std::function<walkGroupsCallback> &func) const override
    {
        for (Group group : m_groups) {
            if (!func(group))
                return false;
        }
        return true;
    }

private:
    QList<Group> m_groups;
};

inline App wildcardApp(const QString &pathsText, bool blocked = false)
{
    App app;
    app.isWildcard = true;
    app.blocked = blocked;
    app.appOriginPath = pathsText;
    return app;
}
