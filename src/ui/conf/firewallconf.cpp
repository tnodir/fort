#include "firewallconf.h"


#include "../util/fileutil.h"
#include "../util/net/netutil.h"
#include "addressgroup.h"
#include "appgroup.h"

FirewallConf::FirewallConf(QObject *parent) :
    QObject(parent),
    m_provBoot(false),
    m_filterEnabled(true),
    m_filterLocals(false),
    m_stopTraffic(false),
    m_stopInetTraffic(false),
    m_resolveAddress(false),
    m_logBlocked(false),
    m_logStat(false),
    m_appBlockAll(true),
    m_appAllowAll(false),
    m_activePeriodEnabled(false),
    m_activePeriodFrom(0),
    m_activePeriodTo(0),
    m_monthStart(DEFAULT_MONTH_START),
    m_trafHourKeepDays(DEFAULT_TRAF_HOUR_KEEP_DAYS),
    m_trafDayKeepDays(DEFAULT_TRAF_DAY_KEEP_DAYS),
    m_trafMonthKeepMonths(DEFAULT_TRAF_MONTH_KEEP_MONTHS),
    m_trafUnit(UnitAdaptive),
    m_quotaDayMb(0),
    m_quotaMonthMb(0)
{
    m_addressGroups.append(new AddressGroup(this));
    m_addressGroups.append(new AddressGroup(this));
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

void FirewallConf::setFilterLocals(bool filterLocals)
{
    if (m_filterLocals != filterLocals) {
        m_filterLocals = filterLocals;
        emit filterLocalsChanged();
    }
}

void FirewallConf::setStopTraffic(bool stopTraffic)
{
    if (m_stopTraffic != stopTraffic) {
        m_stopTraffic = stopTraffic;
        emit stopTrafficChanged();
    }
}

void FirewallConf::setStopInetTraffic(bool stopInetTraffic)
{
    if (m_stopInetTraffic != stopInetTraffic) {
        m_stopInetTraffic = stopInetTraffic;
        emit stopInetTrafficChanged();
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

void FirewallConf::setActivePeriodEnabled(bool activePeriodEnabled)
{
    if (m_activePeriodEnabled != activePeriodEnabled) {
        m_activePeriodEnabled = activePeriodEnabled;
        emit activePeriodEnabledChanged();
    }
}

void FirewallConf::setActivePeriodFrom(int activePeriodFrom)
{
    if (m_activePeriodFrom != activePeriodFrom) {
        m_activePeriodFrom = uint(activePeriodFrom);
        emit activePeriodFromChanged();
    }
}

void FirewallConf::setActivePeriodTo(int activePeriodTo)
{
    if (m_activePeriodTo != activePeriodTo) {
        m_activePeriodTo = uint(activePeriodTo);
        emit activePeriodToChanged();
    }
}

void FirewallConf::setMonthStart(int monthStart)
{
    if (m_monthStart != monthStart) {
        m_monthStart = uint(monthStart);
        emit monthStartChanged();
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

void FirewallConf::setQuotaDayMb(quint32 quotaDayMb)
{
    if (m_quotaDayMb != quotaDayMb) {
        m_quotaDayMb = quotaDayMb;
        emit quotaDayMbChanged();
    }
}

void FirewallConf::setQuotaMonthMb(quint32 quotaMonthMb)
{
    if (m_quotaMonthMb != quotaMonthMb) {
        m_quotaMonthMb = quotaMonthMb;
        emit quotaMonthMbChanged();
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

QQmlListProperty<AddressGroup> FirewallConf::addressGroups()
{
    return {this, m_addressGroups};
}

AppGroup *FirewallConf::appGroupByName(const QString &name) const
{
    foreach (AppGroup *appGroup, appGroupsList()) {
        if (appGroup->name() == name)
            return appGroup;
    }
    return nullptr;
}

QQmlListProperty<AppGroup> FirewallConf::appGroups()
{
    return {this, m_appGroups};
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
    auto appGroup = new AppGroup();
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
    setFilterLocals(o.filterLocals());
    setStopTraffic(o.stopTraffic());
    setStopInetTraffic(o.stopInetTraffic());
    setAppBlockAll(o.appBlockAll());
    setAppAllowAll(o.appAllowAll());
    setPasswordHash(o.passwordHash());
    setAppGroupBits(o.appGroupBits());

    setActivePeriodEnabled(o.activePeriodEnabled());
    setActivePeriodFrom(o.activePeriodFrom());
    setActivePeriodTo(o.activePeriodTo());

    setMonthStart(o.monthStart());
    setTrafHourKeepDays(o.trafHourKeepDays());
    setTrafDayKeepDays(o.trafDayKeepDays());
    setTrafMonthKeepMonths(o.trafMonthKeepMonths());

    setQuotaDayMb(o.quotaDayMb());
    setQuotaMonthMb(o.quotaMonthMb());

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

    QVariantList addresses;
    foreach (const AddressGroup *addressGroup, addressGroupsList()) {
        addresses.append(addressGroup->toVariant());
    }
    map["addressGroups"] = addresses;

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

    const QVariantList addresses = map["addressGroups"].toList();
    int addrGroupIndex = 0;
    foreach (const QVariant &av, addresses) {
        AddressGroup *addressGroup = m_addressGroups.at(addrGroupIndex++);
        addressGroup->fromVariant(av);
    }

    const QVariantList groups = map["appGroups"].toList();
    foreach (const QVariant &gv, groups) {
        auto appGroup = new AppGroup();
        appGroup->fromVariant(gv);
        addAppGroup(appGroup);
    }
}

void FirewallConf::setupDefault()
{
    AddressGroup *inetGroup = inetAddressGroup();
    inetGroup->setExcludeText(NetUtil::localIpv4Networks().join('\n'));

    auto appGroup = new AppGroup();
    appGroup->setName("Main");
    appGroup->setAllowText(FileUtil::appBinLocation() + '/');
    addAppGroup(appGroup);
}
