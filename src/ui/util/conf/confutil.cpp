#include "confutil.h"

#include <conf/appgroup.h>
#include <conf/firewallconf.h>
#include <util/stringutil.h>

namespace {

void writeAppGroupFlags(PFORT_CONF_GROUP out, const FirewallConf &conf)
{
    out->group_bits = 0;
    out->log_blocked = 0;
    out->log_conn = 0;

    int i = 0;
    for (const AppGroup *appGroup : conf.appGroups()) {
        if (appGroup->enabled()) {
            out->group_bits |= (1 << i);
        }
        if (appGroup->logBlocked()) {
            out->log_blocked |= (1 << i);
        }
        if (appGroup->logConn()) {
            out->log_conn |= (1 << i);
        }
        ++i;
    }
}

void writeLimitBps(PFORT_SPEED_LIMIT limit, quint32 kBits)
{
    limit->bps = quint64(kBits) * (1024LL / 8); /* to bytes per second */
}

void writeLimitIn(PFORT_SPEED_LIMIT limit, const AppGroup *appGroup)
{
    limit->plr = appGroup->limitPacketLoss();
    limit->latency_ms = appGroup->limitLatency();
    limit->buffer_bytes = appGroup->limitBufferSizeIn();

    writeLimitBps(limit, appGroup->speedLimitIn());
}

void writeLimitOut(PFORT_SPEED_LIMIT limit, const AppGroup *appGroup)
{
    limit->plr = appGroup->limitPacketLoss();
    limit->latency_ms = appGroup->limitLatency();
    limit->buffer_bytes = appGroup->limitBufferSizeOut();

    writeLimitBps(limit, appGroup->speedLimitOut());
}

void writeLimits(PFORT_CONF_GROUP out, const QList<AppGroup *> &appGroups)
{
    PFORT_SPEED_LIMIT limits = out->limits;

    out->limit_bits = 0;
    out->limit_io_bits = 0;

    const int groupsCount = appGroups.size();
    for (int i = 0; i < groupsCount; ++i, limits += 2) {
        const AppGroup *appGroup = appGroups.at(i);

        const quint32 limitIn = appGroup->enabledSpeedLimitIn();
        const quint32 limitOut = appGroup->enabledSpeedLimitOut();

        const bool isLimitIn = (limitIn != 0);
        const bool isLimitOut = (limitOut != 0);

        if (isLimitIn || isLimitOut) {
            out->limit_bits |= (1 << i);

            if (isLimitIn) {
                out->limit_io_bits |= (1 << (i * 2 + 0));

                writeLimitIn(&limits[0], appGroup);
            }

            if (isLimitOut) {
                out->limit_io_bits |= (1 << (i * 2 + 1));

                writeLimitOut(&limits[1], appGroup);
            }
        }
    }
}

}

int ConfUtil::ruleMaxCount()
{
    return FORT_CONF_RULE_MAX;
}

int ConfUtil::ruleSetMaxCount()
{
    return FORT_CONF_RULE_SET_MAX;
}

int ConfUtil::ruleDepthMaxCount()
{
    return FORT_CONF_RULE_DEPTH_MAX;
}

int ConfUtil::ruleSetDepthMaxCount()
{
    return FORT_CONF_RULE_SET_DEPTH_MAX;
}

int ConfUtil::zoneMaxCount()
{
    return FORT_CONF_ZONE_MAX;
}

QRegularExpressionMatch ConfUtil::matchWildcard(const QStringView &path)
{
    static const QRegularExpression wildMatcher("([*?[])");

    return StringUtil::match(wildMatcher, path);
}

void ConfUtil::migrateZoneData(char **data, const QByteArray &zoneData)
{
    PFORT_CONF_ADDR_LIST addr_list = (PFORT_CONF_ADDR_LIST) zoneData.data();

    if (FORT_CONF_ADDR4_LIST_SIZE(addr_list->ip_n, addr_list->pair_n) == zoneData.size()) {
        IpRange ipRange;
        writeIpRange(data, ipRange, /*isIPv6=*/true);
    }
}

QString ConfUtil::parseAppPath(const QStringView &line, bool &isWild, bool &isPrefix)
{
    auto path = line;
    if (path.startsWith('"') && path.endsWith('"')) {
        path = path.mid(1, path.size() - 2);
    }

    if (path.isEmpty())
        return QString();

    const auto wildMatch = matchWildcard(path);
    if (wildMatch.hasMatch()) {
        if (wildMatch.capturedStart() == path.size() - 2 && path.endsWith(QLatin1String("**"))) {
            path.chop(2);
            isPrefix = true;
        } else {
            isWild = true;
        }
    }

    return path.toString();
}

void ConfUtil::writeConf(char **data, const WriteConfArgs &wca, AppParseOptions &opt)
{
    PFORT_CONF_IO drvConfIo = PFORT_CONF_IO(*data);
    PFORT_CONF drvConf = &drvConfIo->conf;

    quint32 addrGroupsOff;
    quint32 wildAppsOff, prefixAppsOff, exeAppsOff;

    *data = drvConf->data;

#define CONF_DATA_OFFSET quint32(*data - drvConf->data)
    addrGroupsOff = CONF_DATA_OFFSET;
    writeLongs(data, wca.ad.addressGroupOffsets);
    writeAddressRanges(data, wca.ad.addressRanges);

    wildAppsOff = CONF_DATA_OFFSET;
    writeApps(data, opt.wildAppsMap);

    prefixAppsOff = CONF_DATA_OFFSET;
    writeApps(data, opt.prefixAppsMap, /*useHeader=*/true);

    exeAppsOff = CONF_DATA_OFFSET;
    writeApps(data, opt.exeAppsMap);
#undef CONF_DATA_OFFSET

    PFORT_CONF_GROUP conf_group = &drvConfIo->conf_group;

    writeAppGroupFlags(conf_group, wca.conf);

    writeLimits(conf_group, wca.conf.appGroups());

    *data = (char *) &drvConf->flags;
    writeConfFlags(data, wca.conf);

    drvConf->proc_wild = opt.procWild;

    drvConf->wild_apps_n = quint16(opt.wildAppsMap.size());
    drvConf->prefix_apps_n = quint16(opt.prefixAppsMap.size());
    drvConf->exe_apps_n = quint16(opt.exeAppsMap.size());

    drvConf->addr_groups_off = addrGroupsOff;

    drvConf->wild_apps_off = wildAppsOff;
    drvConf->prefix_apps_off = prefixAppsOff;
    drvConf->exe_apps_off = exeAppsOff;
}

void ConfUtil::writeConfFlags(char **data, const FirewallConf &conf)
{
    PFORT_CONF_FLAGS confFlags = PFORT_CONF_FLAGS(*data);

    confFlags->boot_filter = conf.bootFilter();
    confFlags->filter_enabled = conf.filterEnabled();
    confFlags->filter_locals = conf.filterLocals();
    confFlags->filter_local_net = conf.filterLocalNet();

    confFlags->block_traffic = conf.blockTraffic();
    confFlags->block_inet_traffic = conf.blockInetTraffic();
    confFlags->allow_all_new = conf.allowAllNew();
    confFlags->ask_to_connect = conf.askToConnect();
    confFlags->group_blocked = conf.groupBlocked();

    confFlags->app_block_all = conf.appBlockAll();
    confFlags->app_allow_all = conf.appAllowAll();

    confFlags->log_stat = true; // always enabled for driver
    confFlags->log_stat_no_filter = conf.logStatNoFilter();
    confFlags->log_blocked = conf.logBlocked();

    confFlags->log_allowed_ip = conf.logAllowedIp();
    confFlags->log_blocked_ip = conf.logBlockedIp();
    confFlags->log_alerted_blocked_ip = conf.logAlertedBlockedIp();

    confFlags->group_bits = conf.activeGroupBits();
}

void ConfUtil::writeAddressRanges(char **data, const addrranges_arr_t &addressRanges)
{
    for (const AddressRange &addressRange : addressRanges) {
        writeAddressRange(data, addressRange);
    }
}

void ConfUtil::writeAddressRange(char **data, const AddressRange &addressRange)
{
    PFORT_CONF_ADDR_GROUP addrGroup = PFORT_CONF_ADDR_GROUP(*data);

    addrGroup->include_all = addressRange.includeAll();
    addrGroup->exclude_all = addressRange.excludeAll();

    addrGroup->include_zones = addressRange.includeZones();
    addrGroup->exclude_zones = addressRange.excludeZones();

    addrGroup->include_is_empty = addressRange.includeRange().isEmpty();
    addrGroup->exclude_is_empty = addressRange.excludeRange().isEmpty();

    *data += FORT_CONF_ADDR_GROUP_OFF;

    writeAddressList(data, addressRange.includeRange());

    addrGroup->exclude_off = *data - addrGroup->data;

    writeAddressList(data, addressRange.excludeRange());
}

void ConfUtil::writeAddressList(char **data, const IpRange &ipRange)
{
    writeIpRange(data, ipRange);
    writeIpRange(data, ipRange, /*isIPv6=*/true);
}

void ConfUtil::writeIpRange(char **data, const IpRange &ipRange, bool isIPv6)
{
    PFORT_CONF_ADDR_LIST addrList = PFORT_CONF_ADDR_LIST(*data);

    addrList->ip_n = quint32(isIPv6 ? ipRange.ip6Size() : ipRange.ip4Size());
    addrList->pair_n = quint32(isIPv6 ? ipRange.pair6Size() : ipRange.pair4Size());

    *data += FORT_CONF_ADDR_LIST_OFF;

    if (isIPv6) {
        writeIp6Array(data, ipRange.ip6Array());
        writeIp6Array(data, ipRange.pair6FromArray());
        writeIp6Array(data, ipRange.pair6ToArray());
    } else {
        writeLongs(data, ipRange.ip4Array());
        writeLongs(data, ipRange.pair4FromArray());
        writeLongs(data, ipRange.pair4ToArray());
    }
}

bool ConfUtil::loadAddressList(const char **data, IpRange &ipRange, uint &bufSize)
{
    return loadIpRange(data, ipRange, bufSize)
            && (bufSize == 0 || loadIpRange(data, ipRange, bufSize, /*isIPv6=*/true));
}

bool ConfUtil::loadIpRange(const char **data, IpRange &ipRange, uint &bufSize, bool isIPv6)
{
    if (bufSize < FORT_CONF_ADDR_LIST_OFF)
        return false;

    PFORT_CONF_ADDR_LIST addr_list = (PFORT_CONF_ADDR_LIST) *data;
    *data = (const char *) addr_list->ip;

    const uint addrListSize = isIPv6
            ? FORT_CONF_ADDR6_LIST_SIZE(addr_list->ip_n, addr_list->pair_n)
            : FORT_CONF_ADDR4_LIST_SIZE(addr_list->ip_n, addr_list->pair_n);

    if (bufSize < addrListSize)
        return false;

    bufSize -= addrListSize;

    if (isIPv6) {
        ipRange.ip6Array().resize(addr_list->ip_n);
        ipRange.pair6FromArray().resize(addr_list->pair_n);
        ipRange.pair6ToArray().resize(addr_list->pair_n);

        loadIp6Array(data, ipRange.ip6Array());
        loadIp6Array(data, ipRange.pair6FromArray());
        loadIp6Array(data, ipRange.pair6ToArray());
    } else {
        ipRange.ip4Array().resize(addr_list->ip_n);
        ipRange.pair4FromArray().resize(addr_list->pair_n);
        ipRange.pair4ToArray().resize(addr_list->pair_n);

        loadLongs(data, ipRange.ip4Array());
        loadLongs(data, ipRange.pair4FromArray());
        loadLongs(data, ipRange.pair4ToArray());
    }

    return true;
}

void ConfUtil::writeApps(char **data, const appdata_map_t &appsMap, bool useHeader)
{
    quint32 *offp = (quint32 *) *data;
    const quint32 offTableSize = quint32(useHeader ? FORT_CONF_STR_HEADER_SIZE(appsMap.size()) : 0);
    char *p = *data + offTableSize;
    quint32 off = 0;

    if (useHeader) {
        *offp++ = 0;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    for (const auto &[kernelPath, appData] : appsMap.asKeyValueRange()) {
#else
    auto it = appsMap.constBegin();
    for (; it != appsMap.constEnd(); ++it) {
        const auto &kernelPath = it.key();
        const auto &appData = it.value();
#endif
        const int kernelPathSize = kernelPath.size();

        const quint16 appPathLen = quint16(kernelPathSize * sizeof(wchar_t));
        const quint32 appSize = FORT_CONF_APP_ENTRY_SIZE(appPathLen);

        PFORT_APP_ENTRY entry = (PFORT_APP_ENTRY) p;
        entry->app_data = appData;
        entry->path_len = appPathLen;

        kernelPath.toWCharArray(entry->path);
        entry->path[kernelPathSize] = L'\0';

        off += appSize;
        if (useHeader) {
            *offp++ = off;
        }
        p += appSize;
    }

    *data += offTableSize + FORT_CONF_STR_DATA_SIZE(off);
}

void ConfUtil::writeShorts(char **data, const shorts_arr_t &array)
{
    writeData(data, array.constData(), array.size(), sizeof(quint16));
}

void ConfUtil::writeLongs(char **data, const longs_arr_t &array)
{
    writeData(data, array.constData(), array.size(), sizeof(quint32));
}

void ConfUtil::writeIp6Array(char **data, const ip6_arr_t &array)
{
    writeData(data, array.constData(), array.size(), sizeof(ip6_addr_t));
}

void ConfUtil::writeData(char **data, void const *src, int elemCount, uint elemSize)
{
    const size_t arraySize = size_t(elemCount) * elemSize;

    memcpy(*data, src, arraySize);

    *data += arraySize;
}

void ConfUtil::writeChars(char **data, const chars_arr_t &array)
{
    const size_t arraySize = size_t(array.size());

    memcpy(*data, array.constData(), arraySize);

    *data += FORT_CONF_STR_DATA_SIZE(arraySize);
}

void ConfUtil::writeArray(char **data, const QByteArray &array)
{
    const size_t arraySize = size_t(array.size());

    memcpy(*data, array.constData(), arraySize);

    *data += arraySize;
}

void ConfUtil::loadLongs(const char **data, longs_arr_t &array)
{
    loadData(data, array.data(), array.size(), sizeof(quint32));
}

void ConfUtil::loadIp6Array(const char **data, ip6_arr_t &array)
{
    loadData(data, array.data(), array.size(), sizeof(ip6_addr_t));
}

void ConfUtil::loadData(const char **data, void *dst, int elemCount, uint elemSize)
{
    const size_t arraySize = size_t(elemCount) * elemSize;

    memcpy(dst, *data, arraySize);

    *data += arraySize;
}
