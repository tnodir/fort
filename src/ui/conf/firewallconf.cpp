#include "firewallconf.h"

#include "../util/fileutil.h"
#include "../util/net/netutil.h"
#include "addressgroup.h"
#include "appgroup.h"

FirewallConf::FirewallConf(QObject *parent) :
    QObject(parent),
    m_othersEdited(false),
    m_extEdited(false),
    m_graphEdited(false),
    m_iniEdited(false),
    m_flagsEdited(false),
    m_optEdited(false),
    m_startupMode(0),
    m_explorerIntegrated(false),
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

void FirewallConf::setOthersEdited(bool v)
{
    m_othersEdited = v;
}

void FirewallConf::setExtEdited(bool v)
{
    m_extEdited = v;
}

void FirewallConf::setGraphEdited(bool v)
{
    m_graphEdited = v;
}

void FirewallConf::setIniEdited(bool v)
{
    m_iniEdited = v;
}

void FirewallConf::setFlagsEdited(bool v)
{
    m_flagsEdited = v;
}

void FirewallConf::setOptEdited(bool v)
{
    m_optEdited = v;
}

bool FirewallConf::anyEdited() const
{
    return flagsEdited() || iniEdited() || optEdited() || extEdited() || othersEdited();
}

void FirewallConf::resetEdited(bool v)
{
    setOthersEdited(v);
    setExtEdited(v);
    setIniEdited(v);
    setFlagsEdited(v);
    setOptEdited(v);
}

void FirewallConf::setStartupMode(qint8 v)
{
    m_startupMode = v;
}

void FirewallConf::setExplorerIntegrated(bool v)
{
    m_explorerIntegrated = v;
}

void FirewallConf::setProvBoot(bool provBoot)
{
    m_provBoot = provBoot;
}

void FirewallConf::setFilterEnabled(bool filterEnabled)
{
    m_filterEnabled = filterEnabled;
}

void FirewallConf::setFilterLocals(bool filterLocals)
{
    m_filterLocals = filterLocals;
}

void FirewallConf::setStopTraffic(bool stopTraffic)
{
    m_stopTraffic = stopTraffic;
}

void FirewallConf::setStopInetTraffic(bool stopInetTraffic)
{
    m_stopInetTraffic = stopInetTraffic;
}

void FirewallConf::setAllowAllNew(bool allowAllNew)
{
    m_allowAllNew = allowAllNew;
}

void FirewallConf::setLogBlocked(bool logBlocked)
{
    m_logBlocked = logBlocked;
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
    m_logStatNoFilter = logStatNoFilter;
}

void FirewallConf::setLogAllowedIp(bool logAllowedIp)
{
    m_logAllowedIp = logAllowedIp;
}

void FirewallConf::setLogBlockedIp(bool logBlockedIp)
{
    m_logBlockedIp = logBlockedIp;
}

void FirewallConf::setAppBlockAll(bool appBlockAll)
{
    m_appBlockAll = appBlockAll;
}

void FirewallConf::setAppAllowAll(bool appAllowAll)
{
    m_appAllowAll = appAllowAll;
}

void FirewallConf::setActivePeriodEnabled(bool activePeriodEnabled)
{
    m_activePeriodEnabled = activePeriodEnabled;
}

void FirewallConf::setActivePeriodFrom(const QString &activePeriodFrom)
{
    m_activePeriodFrom = activePeriodFrom;
}

void FirewallConf::setActivePeriodTo(const QString &activePeriodTo)
{
    m_activePeriodTo = activePeriodTo;
}

void FirewallConf::setMonthStart(int monthStart)
{
    m_monthStart = monthStart;
}

void FirewallConf::setTrafHourKeepDays(int trafHourKeepDays)
{
    m_trafHourKeepDays = trafHourKeepDays;
}

void FirewallConf::setTrafDayKeepDays(int trafDayKeepDays)
{
    m_trafDayKeepDays = trafDayKeepDays;
}

void FirewallConf::setTrafMonthKeepMonths(int trafMonthKeepMonths)
{
    m_trafMonthKeepMonths = trafMonthKeepMonths;
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
    m_allowedIpKeepCount = allowedIpKeepCount;
}

void FirewallConf::setBlockedIpKeepCount(int blockedIpKeepCount)
{
    m_blockedIpKeepCount = blockedIpKeepCount;
}

void FirewallConf::setQuotaDayMb(quint32 quotaDayMb)
{
    m_quotaDayMb = quotaDayMb;
}

void FirewallConf::setQuotaMonthMb(quint32 quotaMonthMb)
{
    m_quotaMonthMb = quotaMonthMb;
}

quint32 FirewallConf::appGroupBits() const
{
    return m_appGroupBits;
}

void FirewallConf::setAppGroupBits(quint32 groupBits)
{
    m_appGroupBits = groupBits;
}

bool FirewallConf::appGroupEnabled(int groupIndex) const
{
    return (appGroupBits() & (1 << groupIndex)) != 0;
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

void FirewallConf::loadAppGroupBits()
{
    m_appGroupBits = 0;
    int groupIndex = 0;
    for (const AppGroup *appGroup : appGroups()) {
        if (appGroup->enabled()) {
            m_appGroupBits |= (1 << groupIndex);
        }
        ++groupIndex;
    }
}

void FirewallConf::applyAppGroupBits()
{
    int groupIndex = 0;
    for (AppGroup *appGroup : appGroups()) {
        appGroup->setEnabled(appGroupEnabled(groupIndex++));
    }
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

void FirewallConf::prepareToSave()
{
    if (flagsEdited()) {
        loadAppGroupBits();
    }
}

void FirewallConf::copyFlags(const FirewallConf &o)
{
    m_othersEdited = o.othersEdited();
    m_extEdited = o.extEdited();
    m_graphEdited = o.graphEdited();
    m_iniEdited = o.iniEdited();
    m_flagsEdited = o.flagsEdited();
    m_optEdited = o.optEdited();

    m_startupMode = o.startupMode();
    m_explorerIntegrated = o.explorerIntegrated();

    m_provBoot = o.provBoot();
    m_filterEnabled = o.filterEnabled();
    m_filterLocals = o.filterLocals();
    m_stopTraffic = o.stopTraffic();
    m_stopInetTraffic = o.stopInetTraffic();
    m_allowAllNew = o.allowAllNew();

    m_logBlocked = o.logBlocked();
    m_logStat = o.logStat();
    m_logStatNoFilter = o.logStatNoFilter();

    m_logAllowedIp = o.logAllowedIp();
    m_logBlockedIp = o.logBlockedIp();

    m_appBlockAll = o.appBlockAll();
    m_appAllowAll = o.appAllowAll();

    m_activePeriodEnabled = o.activePeriodEnabled();
    m_activePeriodFrom = o.activePeriodFrom();
    m_activePeriodTo = o.activePeriodTo();

    m_monthStart = o.monthStart();
    m_trafHourKeepDays = o.trafHourKeepDays();
    m_trafDayKeepDays = o.trafDayKeepDays();
    m_trafMonthKeepMonths = o.trafMonthKeepMonths();
    m_trafUnit = o.trafUnit();

    m_allowedIpKeepCount = o.allowedIpKeepCount();
    m_blockedIpKeepCount = o.blockedIpKeepCount();

    m_quotaDayMb = o.quotaDayMb();
    m_quotaMonthMb = o.quotaMonthMb();

    m_appGroupBits = o.appGroupBits();
    applyAppGroupBits();
}

void FirewallConf::copy(const FirewallConf &o)
{
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

    copyFlags(o); // after app. groups created
}

QVariant FirewallConf::flagsToVariant() const
{
    QVariantMap map;

    map["othersEdited"] = othersEdited();
    map["extEdited"] = extEdited();
    map["iniEdited"] = iniEdited();
    map["flagsEdited"] = flagsEdited();
    map["optEdited"] = optEdited();

    map["startupMode"] = startupMode();
    map["explorerIntegrated"] = explorerIntegrated();

    map["provBoot"] = provBoot();
    map["filterEnabled"] = filterEnabled();
    map["filterLocals"] = filterLocals();
    map["stopTraffic"] = stopTraffic();
    map["stopInetTraffic"] = stopInetTraffic();
    map["allowAllNew"] = allowAllNew();

    map["logBlocked"] = logBlocked();
    map["logStat"] = logStat();
    map["logStatNoFilter"] = logStatNoFilter();

    map["logAllowedIp"] = logAllowedIp();
    map["logBlockedIp"] = logBlockedIp();

    map["appBlockAll"] = appBlockAll();
    map["appAllowAll"] = appAllowAll();

    map["activePeriodEnabled"] = activePeriodEnabled();
    map["activePeriodFrom"] = activePeriodFrom();
    map["activePeriodTo"] = activePeriodTo();

    map["monthStart"] = monthStart();
    map["trafHourKeepDays"] = trafHourKeepDays();
    map["trafDayKeepDays"] = trafDayKeepDays();
    map["trafMonthKeepMonths"] = trafMonthKeepMonths();
    map["trafUnit"] = trafUnit();

    map["allowedIpKeepCount"] = allowedIpKeepCount();
    map["blockedIpKeepCount"] = blockedIpKeepCount();

    map["quotaDayMb"] = quotaDayMb();
    map["quotaMonthMb"] = quotaMonthMb();

    map["appGroupBits"] = appGroupBits();

    return map;
}

void FirewallConf::flagsFromVariant(const QVariant &v)
{
    const QVariantMap map = v.toMap();

    m_othersEdited = map["othersEdited"].toBool();
    m_extEdited = map["extEdited"].toBool();
    m_graphEdited = map["graphEdited"].toBool();
    m_iniEdited = map["iniEdited"].toBool();
    m_flagsEdited = map["flagsEdited"].toBool();
    m_optEdited = map["optEdited"].toBool();

    m_startupMode = map["startupMode"].toInt();
    m_explorerIntegrated = map["explorerIntegrated"].toBool();

    m_provBoot = map["provBoot"].toBool();
    m_filterEnabled = map["filterEnabled"].toBool();
    m_filterLocals = map["filterLocals"].toBool();
    m_stopTraffic = map["stopTraffic"].toBool();
    m_stopInetTraffic = map["stopInetTraffic"].toBool();
    m_allowAllNew = map["allowAllNew"].toBool();

    m_logBlocked = map["logBlocked"].toBool();
    m_logStat = map["logStat"].toBool();
    m_logStatNoFilter = map["logStatNoFilter"].toBool();

    m_logAllowedIp = map["logAllowedIp"].toBool();
    m_logBlockedIp = map["logBlockedIp"].toBool();

    m_appBlockAll = map["appBlockAll"].toBool();
    m_appAllowAll = map["appAllowAll"].toBool();

    m_activePeriodEnabled = map["activePeriodEnabled"].toBool();
    m_activePeriodFrom = map["activePeriodFrom"].toString();
    m_activePeriodTo = map["activePeriodTo"].toString();

    m_monthStart = map["monthStart"].toInt();
    m_trafHourKeepDays = map["trafHourKeepDays"].toInt();
    m_trafDayKeepDays = map["trafDayKeepDays"].toInt();
    m_trafMonthKeepMonths = map["trafMonthKeepMonths"].toInt();
    m_trafUnit = map["trafUnit"].toInt();

    m_allowedIpKeepCount = map["allowedIpKeepCount"].toInt();
    m_blockedIpKeepCount = map["blockedIpKeepCount"].toInt();

    m_quotaDayMb = map["quotaDayMb"].toUInt();
    m_quotaMonthMb = map["quotaMonthMb"].toUInt();

    setAppGroupBits(map["appGroupBits"].toUInt());
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

    if (!onlyFlags || optEdited()) {
        map["addressGroups"] = addressesToVariant();
        map["appGroups"] = appGroupsToVariant();
    }

    map["flags"] = flagsToVariant();

    return map;
}

void FirewallConf::fromVariant(const QVariant &v, bool onlyFlags)
{
    const QVariantMap map = v.toMap();

    flagsFromVariant(map["flags"]); // set *edited flags

    if (!onlyFlags || optEdited()) {
        addressesFromVariant(map["addressGroups"]);
        appGroupsFromVariant(map["appGroups"]);
    }

    applyAppGroupBits();
}
