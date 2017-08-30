#include "firewallconf.h"

#include "appgroup.h"

FirewallConf::FirewallConf(QObject *parent) :
    QObject(parent),
    m_filterDisabled(false),
    m_ipIncludeAll(false),
    m_ipExcludeAll(false),
    m_appLogBlocked(true),
    m_appBlockAll(true),
    m_appAllowAll(false)
{
}

void FirewallConf::setFilterDisabled(bool filterDisabled)
{
    if (m_filterDisabled != filterDisabled) {
        m_filterDisabled = filterDisabled;
        emit filterDisabledChanged();
    }
}

void FirewallConf::setIpIncludeAll(bool ipIncludeAll)
{
    if (m_ipIncludeAll != ipIncludeAll) {
        m_ipIncludeAll = ipIncludeAll;
        emit ipIncludeAllChanged();
    }
}

void FirewallConf::setIpExcludeAll(bool ipExcludeAll)
{
    if (m_ipExcludeAll != ipExcludeAll) {
        m_ipExcludeAll = ipExcludeAll;
        emit ipExcludeAllChanged();
    }
}

void FirewallConf::setAppLogBlocked(bool appLogBlocked)
{
    if (m_appLogBlocked != appLogBlocked) {
        m_appLogBlocked = appLogBlocked;
        emit appLogBlockedChanged();
    }
}

void FirewallConf::setAppBlockAll(bool appBlockAll)
{
    if (m_appBlockAll != appBlockAll) {
        m_appBlockAll = appBlockAll;
        emit appBlockAllChanged();
    }
}

void FirewallConf::setAppAllowAll(bool appAllowAll)
{
    if (m_appAllowAll != appAllowAll) {
        m_appAllowAll = appAllowAll;
        emit appAllowAllChanged();
    }
}

void FirewallConf::setIpIncludeText(const QString &ipIncludeText)
{
    if (m_ipIncludeText != ipIncludeText) {
        m_ipIncludeText = ipIncludeText;
        emit ipIncludeTextChanged();
    }
}

void FirewallConf::setIpExcludeText(const QString &ipExcludeText)
{
    if (m_ipExcludeText != ipExcludeText) {
        m_ipExcludeText = ipExcludeText;
        emit ipExcludeTextChanged();
    }
}

QQmlListProperty<AppGroup> FirewallConf::appGroups()
{
    return QQmlListProperty<AppGroup>(this, m_appGroups);
}

void FirewallConf::addAppGroup(AppGroup *appGroup, int to)
{
    appGroup->setParent(this);

    if (to < 0) {
        m_appGroups.append(appGroup);
    } else {
        m_appGroups.insert(to, appGroup);
    }
    emit appGroupsChanged();
}

void FirewallConf::moveAppGroup(int from, int to)
{
    m_appGroups.move(from, to);
    emit appGroupsChanged();
}

void FirewallConf::removeAppGroup(int from, int to)
{
    for (int i = to; i >= from; --i) {
        delete m_appGroups.at(i);
        m_appGroups.removeAt(i);
    }
    emit appGroupsChanged();
}

QVariant FirewallConf::toVariant() const
{
    QVariantMap map;

    //map["filterDisabled"] = filterDisabled();
    map["ipIncludeAll"] = ipIncludeAll();
    map["ipExcludeAll"] = ipExcludeAll();
    map["appLogBlocked"] = appLogBlocked();
    map["appBlockAll"] = appBlockAll();
    map["ipIncludeText"] = ipIncludeText();
    map["ipExcludeText"] = ipExcludeText();

    QVariantList groups;
    foreach (const AppGroup *appGroup, appGroupsList()) {
        groups.append(appGroup->toVariant());
    }
    map["appGroups"] = groups;

    return map;
}

void FirewallConf::fromVariant(const QVariant &v)
{
    QVariantMap map = v.toMap();

    //m_filterDisabled = map["filterDisabled"].toBool();
    m_ipIncludeAll = map["ipIncludeAll"].toBool();
    m_ipExcludeAll = map["ipExcludeAll"].toBool();
    m_appLogBlocked = map["appLogBlocked"].toBool();
    m_appBlockAll = map["appBlockAll"].toBool();
    m_ipIncludeText = map["ipIncludeText"].toString();
    m_ipExcludeText = map["ipExcludeText"].toString();

    const QVariantList groups = map["appGroups"].toList();
    foreach (const QVariant &gv, groups) {
        AppGroup *appGroup = new AppGroup();
        appGroup->fromVariant(gv);
        addAppGroup(appGroup);
    }
}
