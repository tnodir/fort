#include "firewallconf.h"

#include "addressgroup.h"
#include "appgroup.h"

FirewallConf::FirewallConf(QObject *parent) :
    QObject(parent),
    m_provBoot(false),
    m_filterEnabled(true),
    m_appLogBlocked(false),
    m_appBlockAll(true),
    m_appAllowAll(false),
    m_ipInclude(new AddressGroup(this)),
    m_ipExclude(new AddressGroup(this))
{
}

void FirewallConf::setProvBoot(bool provBoot)
{
    if (m_provBoot != provBoot) {
        m_provBoot = provBoot;
        emit provBootChanged();
    }
}

void FirewallConf::setFilterEnabled(bool filterEnabled)
{
    if (m_filterEnabled != filterEnabled) {
        m_filterEnabled = filterEnabled;
        emit filterEnabledChanged();
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

quint32 FirewallConf::appGroupBits() const
{
    quint32 groupBits = 0;
    int i = 0;
    foreach (const AppGroup *appGroup, appGroupsList()) {
        if (appGroup->enabled()) {
            groupBits |= (1 << i);
        }
        ++i;
    }
    return groupBits;
}

void FirewallConf::setAppGroupBits(quint32 groupBits)
{
    int i = 0;
    foreach (AppGroup *appGroup, appGroupsList()) {
        appGroup->setEnabled(groupBits & (1 << i));
        ++i;
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

void FirewallConf::addAppGroupByName(const QString &name)
{
    AppGroup *appGroup = new AppGroup();
    appGroup->setName(name);
    addAppGroup(appGroup);
}

void FirewallConf::moveAppGroup(int from, int to)
{
    m_appGroups.move(from, to);
    emit appGroupsChanged();
}

void FirewallConf::removeAppGroup(int from, int to)
{
    for (int i = to; i >= from; --i) {
        AppGroup *appGroup = m_appGroups.at(i);
        appGroup->deleteLater();

        m_appGroups.removeAt(i);
    }
    emit appGroupsChanged();
}

void FirewallConf::copyFlags(const FirewallConf &o)
{
    setProvBoot(o.provBoot());
    setFilterEnabled(o.filterEnabled());
    ipInclude()->setUseAll(o.ipInclude()->useAll());
    ipExclude()->setUseAll(o.ipExclude()->useAll());
    setAppBlockAll(o.appBlockAll());
    setAppAllowAll(o.appAllowAll());
    setAppGroupBits(o.appGroupBits());
}

void FirewallConf::copyTempFlags(const FirewallConf &o)
{
    setAppLogBlocked(o.appLogBlocked());
}

QVariant FirewallConf::toVariant() const
{
    QVariantMap map;

    map["ipInclude"] = ipInclude()->toVariant();
    map["ipExclude"] = ipExclude()->toVariant();

    QVariantList groups;
    foreach (const AppGroup *appGroup, appGroupsList()) {
        groups.append(appGroup->toVariant());
    }
    map["appGroups"] = groups;

    return map;
}

void FirewallConf::fromVariant(const QVariant &v)
{
    const QVariantMap map = v.toMap();

    m_ipInclude->fromVariant(map["ipInclude"]);
    m_ipExclude->fromVariant(map["ipExclude"]);

    const QVariantList groups = map["appGroups"].toList();
    foreach (const QVariant &gv, groups) {
        AppGroup *appGroup = new AppGroup();
        appGroup->fromVariant(gv);
        addAppGroup(appGroup);
    }
}
