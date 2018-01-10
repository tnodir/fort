#include "firewallconf.h"

#include <QCoreApplication>

#include "../util/net/netutil.h"
#include "addressgroup.h"
#include "appgroup.h"

FirewallConf::FirewallConf(QObject *parent) :
    QObject(parent),
    m_provBoot(false),
    m_filterEnabled(true),
    m_stopTraffic(false),
    m_resolveAddress(false),
    m_logBlocked(false),
    m_logStat(false),
    m_appBlockAll(true),
    m_appAllowAll(false),
    m_trafHourKeepDays(DEFAULT_TRAF_HOUR_KEEP_DAYS),
    m_trafDayKeepDays(DEFAULT_TRAF_DAY_KEEP_DAYS),
    m_trafMonthKeepMonths(DEFAULT_TRAF_MONTH_KEEP_MONTHS),
    m_trafUnit(UnitAdaptive),
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

void FirewallConf::setStopTraffic(bool stopTraffic)
{
    if (m_stopTraffic != stopTraffic) {
        m_stopTraffic = stopTraffic;
        emit stopTrafficChanged();
    }
}

void FirewallConf::setResolveAddress(bool resolveAddress)
{
    if (m_resolveAddress != resolveAddress) {
        m_resolveAddress = resolveAddress;
        emit resolveAddressChanged();
    }
}

void FirewallConf::setLogBlocked(bool logBlocked)
{
    if (m_logBlocked != logBlocked) {
        m_logBlocked = logBlocked;
        emit logBlockedChanged();
    }
}

void FirewallConf::setLogStat(bool logStat)
{
    if (m_logStat != logStat) {
        m_logStat = logStat;
        emit logStatChanged();
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

void FirewallConf::setTrafHourKeepDays(int trafHourKeepDays)
{
    if (m_trafHourKeepDays != trafHourKeepDays) {
        m_trafHourKeepDays = trafHourKeepDays;
        emit trafHourKeepDaysChanged();
    }
}

void FirewallConf::setTrafDayKeepDays(int trafDayKeepDays)
{
    if (m_trafDayKeepDays != trafDayKeepDays) {
        m_trafDayKeepDays = trafDayKeepDays;
        emit trafDayKeepDaysChanged();
    }
}

void FirewallConf::setTrafMonthKeepMonths(int trafMonthKeepMonths)
{
    if (m_trafMonthKeepMonths != trafMonthKeepMonths) {
        m_trafMonthKeepMonths = trafMonthKeepMonths;
        emit trafMonthKeepMonthsChanged();
    }
}

void FirewallConf::setTrafUnit(int trafUnit)
{
    if (m_trafUnit != trafUnit) {
        m_trafUnit = static_cast<TrafUnit>(trafUnit);
        emit trafUnitChanged();
    }
}

void FirewallConf::setPasswordHash(const QString &passwordHash)
{
    if (m_passwordHash != passwordHash) {
        m_passwordHash = passwordHash;
        emit passwordHashChanged();
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
    setStopTraffic(o.stopTraffic());
    ipInclude()->setUseAll(o.ipInclude()->useAll());
    ipExclude()->setUseAll(o.ipExclude()->useAll());
    setAppBlockAll(o.appBlockAll());
    setAppAllowAll(o.appAllowAll());
    setPasswordHash(o.passwordHash());
    setAppGroupBits(o.appGroupBits());

    setTrafHourKeepDays(o.trafHourKeepDays());
    setTrafDayKeepDays(o.trafDayKeepDays());
    setTrafMonthKeepMonths(o.trafMonthKeepMonths());

    copyImmediateFlags(o);
}

void FirewallConf::copyImmediateFlags(const FirewallConf &o)
{
    setResolveAddress(o.resolveAddress());
    setLogBlocked(o.logBlocked());
    setLogStat(o.logStat());
    setTrafUnit(o.trafUnit());
}

QVariant FirewallConf::toVariant() const
{
    QVariantMap map;

    map["passwordHash"] = m_passwordHash;

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

    m_passwordHash = map["passwordHash"].toString();

    m_ipInclude->fromVariant(map["ipInclude"]);
    m_ipExclude->fromVariant(map["ipExclude"]);

    const QVariantList groups = map["appGroups"].toList();
    foreach (const QVariant &gv, groups) {
        AppGroup *appGroup = new AppGroup();
        appGroup->fromVariant(gv);
        addAppGroup(appGroup);
    }
}

void FirewallConf::setupDefault()
{
    m_ipInclude->setUseAll(true);
    m_ipExclude->setText(NetUtil::localIpv4Networks().join('\n'));

    AppGroup *appGroup = new AppGroup();
    appGroup->setName("Main");
    appGroup->setAllowText(qApp->applicationDirPath() + '/');
    addAppGroup(appGroup);
}
