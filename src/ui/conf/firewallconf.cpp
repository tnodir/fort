#include "firewallconf.h"

#include <manager/envmanager.h>
#include <util/dateutil.h>
#include <util/net/netutil.h>

#include "addressgroup.h"
#include "appgroup.h"

FirewallConf::FirewallConf(Settings *settings, QObject *parent) : QObject(parent), m_ini(settings)
{
    setupAddressGroups();
}

int FirewallConf::blockTrafficIndex() const
{
    if (m_blockTraffic)
        return BlockTrafficAll;
    if (m_blockInetTraffic)
        return m_blockLanTraffic ? BlockTrafficInetLan : BlockTrafficInet;
    if (m_blockLanTraffic)
        return BlockTrafficLan;
    return BlockTrafficNone;
}

void FirewallConf::setBlockTrafficIndex(int index)
{
    m_blockTraffic = false;
    m_blockLanTraffic = false;
    m_blockInetTraffic = false;

    switch (index) {
    case BlockTrafficAll: { // Block All Traffic
        m_blockTraffic = true;
    } break;
    case BlockTrafficInetLan: { // Block Internet & LAN Traffic
        m_blockInetTraffic = true;
        m_blockLanTraffic = true;
    } break;
    case BlockTrafficInet: { // Block Internet Traffic
        m_blockInetTraffic = true;
    } break;
    case BlockTrafficLan: { // Block LAN Traffic
        m_blockLanTraffic = true;
    } break;
    case BlockTrafficNone: { // Disabled
    } break;
    }
}

FirewallConf::FilterMode FirewallConf::filterMode() const
{
    if (m_allowAllNew)
        return ModeAutoLearn;
    if (m_askToConnect)
        return ModeAskToConnect;
    if (m_appBlockAll)
        return ModeBlockAll;
    if (m_appAllowAll)
        return ModeAllowAll;
    return ModeIgnore;
}

void FirewallConf::setFilterMode(FirewallConf::FilterMode mode)
{
    // TODO: Implement "Ask to Connect" mode
    if (mode == ModeAskToConnect)
        return;

    m_allowAllNew = false;
    m_askToConnect = false;
    m_appBlockAll = false;
    m_appAllowAll = false;

    switch (mode) {
    case ModeAutoLearn: {
        m_allowAllNew = true;
        m_appBlockAll = true;
    } break;
    case ModeAskToConnect: {
        m_askToConnect = true;
        m_appBlockAll = true;
    } break;
    case ModeBlockAll: {
        m_appBlockAll = true;
    } break;
    case ModeAllowAll: {
        m_appAllowAll = true;
    } break;
    }
}

QStringList FirewallConf::blockTrafficNames()
{
    // Sync with BlockTrafficType
    return { tr("No Block"), tr("Block Internet Traffic"), tr("Block LAN Traffic"),
        tr("Block Internet and LAN Traffic"), tr("Block All Traffic") };
}

QStringList FirewallConf::blockTrafficIconPaths()
{
    return { QString(), ":/icons/hostname.png", ":/icons/global_telecom.png",
        ":/icons/computer.png", ":/icons/cross.png" };
}

QStringList FirewallConf::filterModeNames()
{
    // Sync with enum FilterMode
    return { tr("Auto-Learn"), tr("Ask to Connect"), tr("Block, if not allowed"),
        tr("Allow, if not blocked"), tr("Ignore, if not blocked or allowed") };
}

QStringList FirewallConf::filterModeIconPaths()
{
    return { ":/icons/lightbulb.png", ":/icons/help.png", ":/icons/deny.png", ":/icons/accept.png",
        ":/icons/road_sign.png" };
}

void FirewallConf::setupAppGroupBits(quint32 v)
{
    setAppGroupBits(v);
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

void FirewallConf::loadGroupPeriodBits()
{
    const QTime now = DateUtil::currentTime();

    m_anyGroupPeriodEnabled = false;
    m_groupActivePeriodBits = quint32(-1);
    int groupIndex = 0;
    for (AppGroup *appGroup : appGroups()) {
        if (appGroup->enabled() && appGroup->periodEnabled()) {
            m_anyGroupPeriodEnabled = true;

            if (!appGroup->isTimeInPeriod(now)) {
                m_groupActivePeriodBits ^= (1 << groupIndex);
            }
        }
        ++groupIndex;
    }
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

    // COMPAT: Remove after v4.1.0
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

bool FirewallConf::updateGroupPeriods(bool /*onlyFlags*/)
{
    loadGroupPeriodBits();

    return m_anyGroupPeriodEnabled;
}

void FirewallConf::copyFlags(const FirewallConf &o)
{
    m_editedFlags = o.editedFlags();

    m_bootFilter = o.bootFilter();
    m_stealthMode = o.stealthMode();
    m_filterEnabled = o.filterEnabled();
    m_filterLocals = o.filterLocals();
    m_filterLocalNet = o.filterLocalNet();
    m_blockTraffic = o.blockTraffic();
    m_blockLanTraffic = o.blockLanTraffic();
    m_blockInetTraffic = o.blockInetTraffic();
    m_allowAllNew = o.allowAllNew();
    m_askToConnect = o.askToConnect();
    m_groupBlocked = o.groupBlocked();

    m_logStat = o.logStat();
    m_logStatNoFilter = o.logStatNoFilter();
    m_logApp = o.logApp();

    m_logAllowedConn = o.logAllowedConn();
    m_logBlockedConn = o.logBlockedConn();
    m_logAlertedConn = o.logAlertedConn();

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
    map["stealthMode"] = stealthMode();
    map["filterEnabled"] = filterEnabled();
    map["filterLocals"] = filterLocals();
    map["filterLocalNet"] = filterLocalNet();
    map["blockTraffic"] = blockTraffic();
    map["blockLanTraffic"] = blockLanTraffic();
    map["blockInetTraffic"] = blockInetTraffic();
    map["allowAllNew"] = allowAllNew();
    map["askToConnect"] = askToConnect();
    map["groupBlocked"] = groupBlocked();

    map["logStat"] = logStat();
    map["logStatNoFilter"] = logStatNoFilter();
    map["logApp"] = logApp();

    map["logAllowedConn"] = logAllowedConn();
    map["logBlockedConn"] = logBlockedConn();
    map["logAlertedConn"] = logAlertedConn();

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
    m_stealthMode = map["stealthMode"].toBool();
    m_filterEnabled = map["filterEnabled"].toBool();
    m_filterLocals = map["filterLocals"].toBool();
    m_filterLocalNet = map["filterLocalNet"].toBool();
    m_blockTraffic = map["blockTraffic"].toBool();
    m_blockLanTraffic = map["blockLanTraffic"].toBool();
    m_blockInetTraffic = map["blockInetTraffic"].toBool();
    m_allowAllNew = map["allowAllNew"].toBool();
    m_askToConnect = map["askToConnect"].toBool();
    m_groupBlocked = map["groupBlocked"].toBool();

    m_logApp = map["logApp"].toBool();
    m_logStat = map["logStat"].toBool();
    m_logStatNoFilter = map["logStatNoFilter"].toBool();

    m_logAllowedConn = map["logAllowedConn"].toBool();
    m_logBlockedConn = map["logBlockedConn"].toBool();
    m_logAlertedConn = map["logAlertedConn"].toBool();

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

    resetEdited(onlyEdited ? FirewallConf::EditedFlags(editedFlagsFromVariant(v))
                           : FirewallConf::AllEdited);

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
