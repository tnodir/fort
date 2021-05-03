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
    m_logBlocked(false),
    m_logStat(false),
    m_logStatNoFilter(false),
    m_logAllowedIp(false),
    m_logBlockedIp(false),
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
        emit allowAllNewChanged();
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

void FirewallConf::setLogStatNoFilter(bool logStatNoFilter)
{
    if (m_logStatNoFilter != logStatNoFilter) {
        m_logStatNoFilter = logStatNoFilter;
        emit logStatNoFilterChanged();
    }
}

void FirewallConf::setLogAllowedIp(bool logAllowedIp)
{
    if (m_logAllowedIp != logAllowedIp) {
        m_logAllowedIp = logAllowedIp;
        emit logAllowedIpChanged();
    }
}

void FirewallConf::setLogBlockedIp(bool logBlockedIp)
{
    if (m_logBlockedIp != logBlockedIp) {
        m_logBlockedIp = logBlockedIp;
        emit logBlockedIpChanged();
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

void FirewallConf::setAllowedIpKeepCount(int allowedIpKeepCount)
{
    if (m_allowedIpKeepCount != allowedIpKeepCount) {
        m_allowedIpKeepCount = allowedIpKeepCount;
        emit allowedIpKeepCountChanged();
    }
}

void FirewallConf::setBlockedIpKeepCount(int blockedIpKeepCount)
{
    if (m_blockedIpKeepCount != blockedIpKeepCount) {
        m_blockedIpKeepCount = blockedIpKeepCount;
        emit blockedIpKeepCountChanged();
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
    auto appGroup = !m_removedAppGroups.isEmpty() ? m_removedAppGroups.takeLast() : new AppGroup();
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

void FirewallConf::copyImmediateFlags(const FirewallConf &o)
{
    m_logBlocked = o.logBlocked();
    m_logStat = o.logStat();
    m_logStatNoFilter = o.logStatNoFilter();
    m_logAllowedIp = o.logAllowedIp();
    m_logBlockedIp = o.logBlockedIp();
    m_trafUnit = o.trafUnit();
}

void FirewallConf::copyFlags(const FirewallConf &o)
{
    m_provBoot = o.provBoot();
    m_filterEnabled = o.filterEnabled();
    m_filterLocals = o.filterLocals();
    m_stopTraffic = o.stopTraffic();
    m_stopInetTraffic = o.stopInetTraffic();
    m_allowAllNew = o.allowAllNew();
    m_appBlockAll = o.appBlockAll();
    m_appAllowAll = o.appAllowAll();

    setAppGroupBits(o.appGroupBits());

    m_activePeriodEnabled = o.activePeriodEnabled();
    m_activePeriodFrom = o.activePeriodFrom();
    m_activePeriodTo = o.activePeriodTo();

    m_monthStart = o.monthStart();
    m_trafHourKeepDays = o.trafHourKeepDays();
    m_trafDayKeepDays = o.trafDayKeepDays();
    m_trafMonthKeepMonths = o.trafMonthKeepMonths();

    m_allowedIpKeepCount = o.allowedIpKeepCount();
    m_blockedIpKeepCount = o.blockedIpKeepCount();

    m_quotaDayMb = o.quotaDayMb();
    m_quotaMonthMb = o.quotaMonthMb();

    copyImmediateFlags(o);
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

QVariant FirewallConf::immediateFlagsToVariant() const
{
    QVariantMap map;

    map["logBlocked"] = logBlocked();
    map["logStat"] = logStat();
    map["logStatNoFilter"] = logStatNoFilter();
    map["logAllowedIp"] = logAllowedIp();
    map["logBlockedIp"] = logBlockedIp();
    map["trafUnit"] = trafUnit();

    return map;
}

void FirewallConf::immediateFlagsFromVariant(const QVariant &v)
{
    const QVariantMap map = v.toMap();

    m_logBlocked = map["logBlocked"].toBool();
    m_logStat = map["logStat"].toBool();
    m_logStatNoFilter = map["logStatNoFilter"].toBool();
    m_logAllowedIp = map["logAllowedIp"].toBool();
    m_logBlockedIp = map["logBlockedIp"].toBool();
    m_trafUnit = map["trafUnit"].toInt();
}

QVariant FirewallConf::flagsToVariant() const
{
    QVariantMap map = immediateFlagsToVariant().toMap();

    map["provBoot"] = provBoot();
    map["filterEnabled"] = filterEnabled();
    map["filterLocals"] = filterLocals();
    map["stopTraffic"] = stopTraffic();
    map["stopInetTraffic"] = stopInetTraffic();
    map["allowAllNew"] = allowAllNew();
    map["appBlockAll"] = appBlockAll();
    map["appAllowAll"] = appAllowAll();

    map["appGroupBits"] = appGroupBits();

    map["activePeriodEnabled"] = activePeriodEnabled();
    map["activePeriodFrom"] = activePeriodFrom();
    map["activePeriodTo"] = activePeriodTo();

    map["monthStart"] = monthStart();
    map["trafHourKeepDays"] = trafHourKeepDays();
    map["trafDayKeepDays"] = trafDayKeepDays();
    map["trafMonthKeepMonths"] = trafMonthKeepMonths();

    map["allowedIpKeepCount"] = allowedIpKeepCount();
    map["blockedIpKeepCount"] = blockedIpKeepCount();

    map["quotaDayMb"] = quotaDayMb();
    map["quotaMonthMb"] = quotaMonthMb();

    return map;
}

void FirewallConf::flagsFromVariant(const QVariant &v)
{
    const QVariantMap map = v.toMap();

    m_provBoot = map["provBoot"].toBool();
    m_filterEnabled = map["filterEnabled"].toBool();
    m_filterLocals = map["filterLocals"].toBool();
    m_stopTraffic = map["stopTraffic"].toBool();
    m_stopInetTraffic = map["stopInetTraffic"].toBool();
    m_allowAllNew = map["allowAllNew"].toBool();
    m_appBlockAll = map["appBlockAll"].toBool();
    m_appAllowAll = map["appAllowAll"].toBool();

    setAppGroupBits(map["appGroupBits"].toUInt());

    m_activePeriodEnabled = map["activePeriodEnabled"].toBool();
    m_activePeriodFrom = map["activePeriodFrom"].toString();
    m_activePeriodTo = map["activePeriodTo"].toString();

    m_monthStart = map["monthStart"].toInt();
    m_trafHourKeepDays = map["trafHourKeepDays"].toInt();
    m_trafDayKeepDays = map["trafDayKeepDays"].toInt();
    m_trafMonthKeepMonths = map["trafMonthKeepMonths"].toInt();

    m_allowedIpKeepCount = map["allowedIpKeepCount"].toInt();
    m_blockedIpKeepCount = map["blockedIpKeepCount"].toInt();

    m_quotaDayMb = map["quotaDayMb"].toUInt();
    m_quotaMonthMb = map["quotaMonthMb"].toUInt();

    immediateFlagsFromVariant(v);
}

QVariant FirewallConf::addressesToVariant() const
{
    QVariantList addresses;
    for (const AddressGroup *addressGroup : addressGroups()) {
        addresses.append(addressGroup->toVariant());
    }
    return addresses;
}

void FirewallConf::addressesFromVariant(const QVariant &v)
{
    const QVariantList addresses = v.toList();
    int addrGroupIndex = 0;
    for (const QVariant &av : addresses) {
        AddressGroup *addressGroup = m_addressGroups.at(addrGroupIndex++);
        addressGroup->fromVariant(av);
    }
}

QVariant FirewallConf::appGroupsToVariant() const
{
    QVariantList groups;
    for (const AppGroup *appGroup : appGroups()) {
        groups.append(appGroup->toVariant());
    }
    return groups;
}

void FirewallConf::appGroupsFromVariant(const QVariant &v)
{
    const QVariantList groups = v.toList();
    for (const QVariant &gv : groups) {
        auto appGroup = new AppGroup();
        appGroup->fromVariant(gv);
        addAppGroup(appGroup);
    }
}

QVariant FirewallConf::toVariant(bool onlyFlags) const
{
    QVariantMap map;

    if (!onlyFlags) {
        map["addressGroups"] = addressesToVariant();
        map["appGroups"] = appGroupsToVariant();
    }

    map["flags"] = flagsToVariant();

    return map;
}

void FirewallConf::fromVariant(const QVariant &v, bool onlyFlags)
{
    const QVariantMap map = v.toMap();

    if (!onlyFlags) {
        addressesFromVariant(map["addressGroups"]);
        appGroupsFromVariant(map["appGroups"]);
    }

    flagsFromVariant(map["flags"]); // after app. groups created
}
