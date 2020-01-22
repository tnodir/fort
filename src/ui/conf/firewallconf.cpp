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
    m_allowAllNew(false),
    m_resolveAddress(false),
    m_logBlocked(false),
    m_logStat(false),
    m_appBlockAll(true),
    m_appAllowAll(false),
    m_activePeriodEnabled(false)
{
    setupAddressGroups();
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

void FirewallConf::setAllowAllNew(bool allowAllNew)
{
    if (m_allowAllNew != allowAllNew) {
        m_allowAllNew = allowAllNew;
        emit tempAllowAllChanged();
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

void FirewallConf::setActivePeriodFrom(const QString &activePeriodFrom)
{
    if (m_activePeriodFrom != activePeriodFrom) {
        m_activePeriodFrom = activePeriodFrom;
        emit activePeriodFromChanged();
    }
}

void FirewallConf::setActivePeriodTo(const QString &activePeriodTo)
{
    if (m_activePeriodTo != activePeriodTo) {
        m_activePeriodTo = activePeriodTo;
        emit activePeriodToChanged();
    }
}

void FirewallConf::setMonthStart(int monthStart)
{
    if (m_monthStart != monthStart) {
        m_monthStart = monthStart;
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
        m_trafUnit = trafUnit;
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

quint32 FirewallConf::appGroupBits() const
{
    quint32 groupBits = 0;
    int i = 0;
    for (const AppGroup *appGroup : appGroups()) {
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
    for (AppGroup *appGroup : appGroups()) {
        appGroup->setEnabled(groupBits & (1 << i));
        ++i;
    }
}

AppGroup *FirewallConf::appGroupByName(const QString &name) const
{
    for (AppGroup *appGroup : appGroups()) {
        if (appGroup->name() == name)
            return appGroup;
    }
    return nullptr;
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

AppGroup *FirewallConf::addAppGroupByName(const QString &name)
{
    auto appGroup = !m_removedAppGroups.isEmpty()
            ? m_removedAppGroups.takeLast()
            : new AppGroup();
    appGroup->setName(name);
    addAppGroup(appGroup);
    return appGroup;
}

void FirewallConf::moveAppGroup(int from, int to)
{
    const int lo = qMin(from, to);
    const int hi = qMax(from, to);
    for (int i = lo; i >= hi; --i) {
        m_appGroups.at(i)->setEdited(true);
    }

    m_appGroups.move(from, to);
    emit appGroupsChanged();
}

void FirewallConf::removeAppGroup(int from, int to)
{
    const int lo = qMin(from, to);
    const int hi = qMax(from, to);
    for (int i = hi; i >= lo; --i) {
        AppGroup *appGroup = m_appGroups.at(i);
        if (appGroup->id() == 0) {
            appGroup->deleteLater();
        } else {
            appGroup->clear();
            m_removedAppGroups.append(appGroup);
        }

        m_appGroups.removeAt(i);
    }

    if (m_appGroups.isEmpty()) {
        addDefaultAppGroup();
    }
    emit appGroupsChanged();
}

void FirewallConf::addDefaultAppGroup()
{
    auto appGroup = addAppGroupByName("Main");
    appGroup->setAllowText(FileUtil::appBinLocation() + "/**");
}

void FirewallConf::clearRemovedAppGroups() const
{
    qDeleteAll(m_removedAppGroups);
    m_removedAppGroups.clear();
}

void FirewallConf::setupDefaultAddressGroups()
{
    AddressGroup *inetGroup = inetAddressGroup();
    inetGroup->setExcludeText(NetUtil::localIpv4Networks().join('\n'));
}

void FirewallConf::setupAddressGroups()
{
    m_addressGroups.append(new AddressGroup(this));
    m_addressGroups.append(new AddressGroup(this));
}

void FirewallConf::copyFlags(const FirewallConf &o)
{
    setProvBoot(o.provBoot());
    setFilterEnabled(o.filterEnabled());
    setFilterLocals(o.filterLocals());
    setStopTraffic(o.stopTraffic());
    setStopInetTraffic(o.stopInetTraffic());
    setAllowAllNew(o.allowAllNew());
    setAppBlockAll(o.appBlockAll());
    setAppAllowAll(o.appAllowAll());
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

void FirewallConf::copy(const FirewallConf &o)
{
    copyFlags(o);

    int addrGroupIndex = 0;
    for (const AddressGroup *ag : o.addressGroups()) {
        AddressGroup *addressGroup = m_addressGroups.at(addrGroupIndex++);
        addressGroup->copy(*ag);
    }

    for (const AppGroup *ag : o.appGroups()) {
        auto appGroup = new AppGroup();
        appGroup->copy(*ag);
        addAppGroup(appGroup);
    }
}
