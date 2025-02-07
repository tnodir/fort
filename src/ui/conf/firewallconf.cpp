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

    if (mode != ModeAutoLearn && m_allowAllNew) {
        setAutoLearnOff();
    }

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

bool FirewallConf::appGroupEnabled(quint8 groupId) const
{
    const quint64 groupBit = (1ULL << groupId);

    return (appGroupsMask() & groupBit) != 0;
}

void FirewallConf::setAppGroupEnabled(quint8 groupId, bool v)
{
    const quint64 groupBit = (1ULL << groupId);

    if (v) {
        m_appGroupsMask |= groupBit;
    } else {
        m_appGroupsMask &= ~groupBit;
    }
}

void FirewallConf::loadGroupPeriodsMask()
{
    const QTime now = DateUtil::currentTime();

    m_anyGroupPeriodEnabled = false;
    m_groupActivePeriodsMask = quint64(-1LL);

#if 0
    int groupId = 1;
    for (AppGroup *appGroup : appGroups()) {
        if (appGroup->enabled() && appGroup->periodEnabled()) {
            m_anyGroupPeriodEnabled = true;

            if (!appGroup->isTimeInPeriod(now)) {
                const quint64 groupBit = (1ULL << groupId);

                m_groupActivePeriodsMask ^= groupBit;
            }
        }
        ++groupId;
    }
#endif
}

void FirewallConf::setupDefaultAddressGroups()
{
    // Internet Addresses
    {
        AddressGroup *inetGroup = inetAddressGroup();
        inetGroup->setId(1);
        inetGroup->setExcludeText(NetUtil::localIpNetworksText());
    }

    // Internet Block Addresses
    {
        AddressGroup *blockGroup = blockAddressGroup();
        blockGroup->setId(2);
    }
}

void FirewallConf::setupAddressGroups()
{
    m_addressGroups.append(new AddressGroup(this));
    m_addressGroups.append(new AddressGroup(this));
}

void FirewallConf::afterSaved()
{
    ini().clear();
}

bool FirewallConf::updateGroupPeriods(bool /*onlyFlags*/)
{
    loadGroupPeriodsMask();

    return m_anyGroupPeriodEnabled;
}

void FirewallConf::copyFlags(const FirewallConf &o)
{
    m_editedFlags = o.editedFlags();

    m_bootFilter = o.bootFilter();
    m_stealthMode = o.stealthMode();
    m_traceEvents = o.traceEvents();
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

    m_appGroupsMask = o.appGroupsMask();
}

void FirewallConf::copy(const FirewallConf &o)
{
    int addrGroupIndex = 0;
    for (const AddressGroup *ag : o.addressGroups()) {
        AddressGroup *addressGroup = m_addressGroups.at(addrGroupIndex++);
        addressGroup->copy(*ag);
    }

    copyFlags(o); // after app. groups created
}

QVariant FirewallConf::flagsToVariant() const
{
    QVariantMap map;

    map["bootFilter"] = bootFilter();
    map["stealthMode"] = stealthMode();
    map["traceEvents"] = traceEvents();
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

    map["appGroupsMask"] = appGroupsMask();

    return map;
}

void FirewallConf::flagsFromVariant(const QVariant &v)
{
    const QVariantMap map = v.toMap();

    m_bootFilter = map["bootFilter"].toBool();
    m_stealthMode = map["stealthMode"].toBool();
    m_traceEvents = map["traceEvents"].toBool();
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

    m_appGroupsMask = map["appGroupsMask"].toULongLong();
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

QVariant FirewallConf::toVariant(bool onlyEdited) const
{
    QVariantMap map;

    const EditedFlags flags = onlyEdited ? editedFlags() : AllEdited;

    if (onlyEdited) {
        map = editedFlagsToVariant(flags).toMap();
    }

    if ((flags & OptEdited) != 0) {
        map["addressGroups"] = addressesToVariant();
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
