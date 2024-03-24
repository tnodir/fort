#include "firewallconf.h"

#include <manager/envmanager.h>
#include <util/net/netutil.h>

#include "addressgroup.h"
#include "appgroup.h"

FirewallConf::FirewallConf(Settings *settings, QObject *parent) : QObject(parent), m_ini(settings)
{
    setupAddressGroups();
}

void FirewallConf::resetEdited(bool v)
{
    m_editedFlags = v ? AllEdited : NoneEdited;
}

void FirewallConf::setBootFilter(bool bootFilter)
{
    m_bootFilter = bootFilter;
}

void FirewallConf::setFilterEnabled(bool filterEnabled)
{
    m_filterEnabled = filterEnabled;
}

void FirewallConf::setFilterLocals(bool filterLocals)
{
    m_filterLocals = filterLocals;
}

void FirewallConf::setBlockTraffic(bool blockTraffic)
{
    m_blockTraffic = blockTraffic;
}

void FirewallConf::setBlockInetTraffic(bool blockInetTraffic)
{
    m_blockInetTraffic = blockInetTraffic;
}

void FirewallConf::setAllowAllNew(bool allowAllNew)
{
    m_allowAllNew = allowAllNew;
}

void FirewallConf::setAskToConnect(bool askToConnect)
{
    m_askToConnect = askToConnect;
}

void FirewallConf::setLogStat(bool logStat)
{
    m_logStat = logStat;
}

void FirewallConf::setLogStatNoFilter(bool logStatNoFilter)
{
    m_logStatNoFilter = logStatNoFilter;
}

void FirewallConf::setLogBlocked(bool logBlocked)
{
    m_logBlocked = logBlocked;
}

void FirewallConf::setLogAllowedIp(bool logAllowedIp)
{
    m_logAllowedIp = logAllowedIp;
}

void FirewallConf::setLogBlockedIp(bool logBlockedIp)
{
    m_logBlockedIp = logBlockedIp;
}

void FirewallConf::setLogAlertedBlockedIp(bool logAlertedBlockedIp)
{
    m_logAlertedBlockedIp = logAlertedBlockedIp;
}

void FirewallConf::setAppBlockAll(bool appBlockAll)
{
    m_appBlockAll = appBlockAll;
}

void FirewallConf::setAppAllowAll(bool appAllowAll)
{
    m_appAllowAll = appAllowAll;
}

int FirewallConf::filterModeIndex() const
{
    return m_allowAllNew ? 0 : (m_askToConnect ? 1 : (m_appBlockAll ? 2 : (m_appAllowAll ? 3 : 4)));
}

void FirewallConf::setFilterModeIndex(int index)
{
    m_allowAllNew = false;
    m_askToConnect = false;
    m_appBlockAll = false;
    m_appAllowAll = false;

    switch (index) {
    case 0: { // Auto-Learn
        m_allowAllNew = true;
        m_appBlockAll = true;
    } break;
    case 1: { // Ask
        m_askToConnect = true;
        m_appBlockAll = true;
    } break;
    case 2: { // Block
        m_appBlockAll = true;
    } break;
    case 3: { // Allow
        m_appAllowAll = true;
    } break;
    }
}

QStringList FirewallConf::filterModeNames()
{
    return { tr("Auto-Learn"), tr("Ask to Connect"), tr("Block, if not allowed"),
        tr("Allow, if not blocked"), tr("Ignore, if not blocked or allowed") };
}

QStringList FirewallConf::filterModeIconPaths()
{
    return { ":/icons/lightbulb.png", ":/icons/help.png", ":/icons/deny.png", ":/icons/accept.png",
        ":/icons/road_sign.png" };
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

void FirewallConf::setAppGroupBits(quint32 groupBits)
{
    m_appGroupBits = groupBits;
}

void FirewallConf::setupAppGroupBits(quint32 groupBits)
{
    setAppGroupBits(groupBits);
    applyAppGroupBits();
}

bool FirewallConf::appGroupEnabled(int groupIndex) const
{
    return (appGroupBits() & (1 << groupIndex)) != 0;
}

const AppGroup *FirewallConf::appGroupAt(int index) const
{
    if (index < 0 || index >= appGroups().size()) {
        static const AppGroup g_nullAppGroup;
        return &g_nullAppGroup;
    }

    const AppGroup *appGroup = appGroups().at(index);
    return appGroup;
}

QStringList FirewallConf::appGroupNames() const
{
    QStringList list;
    for (const auto &appGroup : std::as_const(appGroups())) {
        list.append(appGroup->name());
    }
    return list;
}

AppGroup *FirewallConf::appGroupByName(const QString &name) const
{
    for (AppGroup *appGroup : appGroups()) {
        if (appGroup->name() == name)
            return appGroup;
    }
    return nullptr;
}

bool FirewallConf::checkDeprecatedAppGroups() const
{
    for (AppGroup *appGroup : appGroups()) {
        if (appGroup->hasAnyText())
            return false;
    }
    return true;
}

void FirewallConf::addAppGroup(AppGroup *appGroup)
{
    appGroup->setParent(this);

    m_appGroups.append(appGroup);

    emit appGroupsChanged();
}

AppGroup *FirewallConf::addAppGroupByName(const QString &name)
{
    AppGroup *appGroup = new AppGroup();
    appGroup->setId(m_removedAppGroupIdList.isEmpty() ? 0 : m_removedAppGroupIdList.takeLast());
    appGroup->setName(name);
    appGroup->setEdited(true);

    addAppGroup(appGroup);

    return appGroup;
}

void FirewallConf::addDefaultAppGroup()
{
    addAppGroupByName("Main");
}

void FirewallConf::moveAppGroup(int from, int to)
{
    m_appGroups.move(from, to);

    setAppGroupsEdited(from, to);
}

void FirewallConf::removeAppGroup(int from, int to)
{
    const int lo = qMin(from, to);
    const int hi = qMax(from, to);
    for (int i = hi; i >= lo; --i) {
        AppGroup *appGroup = m_appGroups.at(i);
        if (appGroup->id() > 0) {
            m_removedAppGroupIdList.append(appGroup->id());
        }
        appGroup->deleteLater();

        m_appGroups.removeAt(i);
    }

    setAppGroupsEdited(lo, m_appGroups.size() - 1);
}

void FirewallConf::clearRemovedAppGroupIdList() const
{
    m_removedAppGroupIdList.clear();
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
    inetGroup->setExcludeText(NetUtil::localIpNetworksText());
}

void FirewallConf::setupAddressGroups()
{
    m_addressGroups.append(new AddressGroup(this));
    m_addressGroups.append(new AddressGroup(this));
}

void FirewallConf::setAppGroupsEdited(int from, int to)
{
    const int lo = qMin(from, to);
    const int hi = qMax(from, to);

    for (int i = lo; i <= hi; ++i) {
        AppGroup *appGroup = m_appGroups.at(i);
        appGroup->setEdited(true);
    }

    emit appGroupsChanged();
}

void FirewallConf::prepareToSave()
{
    if (flagsEdited()) {
        loadAppGroupBits();
    }
}

void FirewallConf::afterSaved()
{
    ini().clear();
}

void FirewallConf::copyFlags(const FirewallConf &o)
{
    m_editedFlags = o.editedFlags();

    m_bootFilter = o.bootFilter();
    m_filterEnabled = o.filterEnabled();
    m_filterLocals = o.filterLocals();
    m_blockTraffic = o.blockTraffic();
    m_blockInetTraffic = o.blockInetTraffic();
    m_allowAllNew = o.allowAllNew();
    m_askToConnect = o.askToConnect();

    m_logStat = o.logStat();
    m_logStatNoFilter = o.logStatNoFilter();
    m_logBlocked = o.logBlocked();

    m_logAllowedIp = o.logAllowedIp();
    m_logBlockedIp = o.logBlockedIp();
    m_logAlertedBlockedIp = o.logAlertedBlockedIp();

    m_appBlockAll = o.appBlockAll();
    m_appAllowAll = o.appAllowAll();

    m_activePeriodEnabled = o.activePeriodEnabled();
    m_activePeriodFrom = o.activePeriodFrom();
    m_activePeriodTo = o.activePeriodTo();

    setupAppGroupBits(o.appGroupBits());
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

    map["bootFilter"] = bootFilter();
    map["filterEnabled"] = filterEnabled();
    map["filterLocals"] = filterLocals();
    map["blockTraffic"] = blockTraffic();
    map["blockInetTraffic"] = blockInetTraffic();
    map["allowAllNew"] = allowAllNew();
    map["askToConnect"] = askToConnect();

    map["logStat"] = logStat();
    map["logStatNoFilter"] = logStatNoFilter();
    map["logBlocked"] = logBlocked();

    map["logAllowedIp"] = logAllowedIp();
    map["logBlockedIp"] = logBlockedIp();
    map["logAlertedBlockedIp"] = logAlertedBlockedIp();

    map["appBlockAll"] = appBlockAll();
    map["appAllowAll"] = appAllowAll();

    map["activePeriodEnabled"] = activePeriodEnabled();
    map["activePeriodFrom"] = activePeriodFrom();
    map["activePeriodTo"] = activePeriodTo();

    map["appGroupBits"] = appGroupBits();

    return map;
}

void FirewallConf::flagsFromVariant(const QVariant &v)
{
    const QVariantMap map = v.toMap();

    m_bootFilter = map["bootFilter"].toBool();
    m_filterEnabled = map["filterEnabled"].toBool();
    m_filterLocals = map["filterLocals"].toBool();
    m_blockTraffic = map["blockTraffic"].toBool();
    m_blockInetTraffic = map["blockInetTraffic"].toBool();
    m_allowAllNew = map["allowAllNew"].toBool();
    m_askToConnect = map["askToConnect"].toBool();

    m_logBlocked = map["logBlocked"].toBool();
    m_logStat = map["logStat"].toBool();
    m_logStatNoFilter = map["logStatNoFilter"].toBool();

    m_logAllowedIp = map["logAllowedIp"].toBool();
    m_logBlockedIp = map["logBlockedIp"].toBool();
    m_logAlertedBlockedIp = map["logAlertedBlockedIp"].toBool();

    m_appBlockAll = map["appBlockAll"].toBool();
    m_appAllowAll = map["appAllowAll"].toBool();

    m_activePeriodEnabled = map["activePeriodEnabled"].toBool();
    m_activePeriodFrom = map["activePeriodFrom"].toString();
    m_activePeriodTo = map["activePeriodTo"].toString();

    setupAppGroupBits(map["appGroupBits"].toUInt());
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

QVariant FirewallConf::removedAppGroupIdListToVariant() const
{
    QVariantList list;
    for (const qint64 id : removedAppGroupIdList()) {
        list.append(id);
    }
    return list;
}

void FirewallConf::removedAppGroupIdListFromVariant(const QVariant &v)
{
    const QVariantList list = v.toList();
    for (const QVariant &v : list) {
        m_removedAppGroupIdList.append(v.toLongLong());
    }
}

QVariant FirewallConf::toVariant(bool onlyEdited) const
{
    QVariantMap map;

    const EditedFlags flags = onlyEdited ? editedFlags() : AllEdited;

    if (onlyEdited) {
        map = editedFlagsToVariant(flags).toMap();
    }

    if ((flags & OptEdited) != 0) {
        map["addressGroups"] = addressesToVariant();

        map["appGroups"] = appGroupsToVariant();
        map["removedAppGroupIdList"] = removedAppGroupIdListToVariant();
    }

    if ((flags & FlagsEdited) != 0) {
        map["flags"] = flagsToVariant();
    }

    if ((flags & (IniEdited | TaskEdited)) != 0) {
        const QVariantMap iniMap = ini().map();
        if (!iniMap.isEmpty()) {
            map["ini"] = iniMap;
        }
    }

    return map;
}

void FirewallConf::fromVariant(const QVariant &v, bool onlyEdited)
{
    const QVariantMap map = v.toMap();

    if (onlyEdited) {
        m_editedFlags = editedFlagsFromVariant(v);
    } else {
        resetEdited(true);
    }

    if (optEdited()) {
        addressesFromVariant(map["addressGroups"]);

        appGroupsFromVariant(map["appGroups"]);
        removedAppGroupIdListFromVariant(map["removedAppGroupIdList"]);
    }

    if (flagsEdited()) {
        flagsFromVariant(map["flags"]);
    }

    if (iniEdited() || taskEdited()) {
        ini().setMap(map["ini"].toMap());
    }
}

QVariant FirewallConf::editedFlagsToVariant(uint editedFlags)
{
    QVariantMap map;

    map["editedFlags"] = editedFlags;

    return map;
}

uint FirewallConf::editedFlagsFromVariant(const QVariant &v)
{
    const QVariantMap map = v.toMap();
    return map["editedFlags"].toUInt();
}
