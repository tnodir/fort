#include "confdata.h"

#include <conf/appgroup.h>
#include <conf/firewallconf.h>

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

ConfData::ConfData(void *data) : m_data((char *) data), m_base((char *) data) { }

void ConfData::writeConf(const WriteConfArgs &wca, AppParseOptions &opt)
{
    PFORT_CONF_IO drvConfIo = PFORT_CONF_IO(m_data);
    PFORT_CONF drvConf = &drvConfIo->conf;

    quint32 addrGroupsOff;
    quint32 wildAppsOff, prefixAppsOff, exeAppsOff;

    m_data = drvConf->data;
    resetBase();

    addrGroupsOff = dataOffset();
    writeLongs(wca.ad.addressGroupOffsets);
    writeAddressRanges(wca.ad.addressRanges);

    wildAppsOff = dataOffset();
    writeApps(opt.wildAppsMap);

    prefixAppsOff = dataOffset();
    writeApps(opt.prefixAppsMap, /*useHeader=*/true);

    exeAppsOff = dataOffset();
    writeApps(opt.exeAppsMap);

    PFORT_CONF_GROUP conf_group = &drvConfIo->conf_group;

    writeAppGroupFlags(conf_group, wca.conf);

    writeLimits(conf_group, wca.conf.appGroups());

    ConfData(&drvConf->flags).writeConfFlags(wca.conf);

    drvConf->proc_wild = opt.procWild;

    drvConf->wild_apps_n = quint16(opt.wildAppsMap.size());
    drvConf->prefix_apps_n = quint16(opt.prefixAppsMap.size());
    drvConf->exe_apps_n = quint16(opt.exeAppsMap.size());

    drvConf->addr_groups_off = addrGroupsOff;

    drvConf->wild_apps_off = wildAppsOff;
    drvConf->prefix_apps_off = prefixAppsOff;
    drvConf->exe_apps_off = exeAppsOff;
}

void ConfData::writeConfFlags(const FirewallConf &conf)
{
    PFORT_CONF_FLAGS confFlags = PFORT_CONF_FLAGS(m_data);

    confFlags->boot_filter = conf.bootFilter();
    confFlags->filter_enabled = conf.filterEnabled();
    confFlags->filter_locals = conf.filterLocals();
    confFlags->filter_local_net = conf.filterLocalNet();

    confFlags->block_traffic = conf.blockTraffic();
    confFlags->block_lan_traffic = conf.blockLanTraffic();
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

void ConfData::writeAddressRanges(const addrranges_arr_t &addressRanges)
{
    for (const AddressRange &addressRange : addressRanges) {
        writeAddressRange(addressRange);
    }
}

void ConfData::writeAddressRange(const AddressRange &addressRange)
{
    PFORT_CONF_ADDR_GROUP addrGroup = PFORT_CONF_ADDR_GROUP(m_data);

    addrGroup->include_all = addressRange.includeAll();
    addrGroup->exclude_all = addressRange.excludeAll();

    addrGroup->include_zones = addressRange.includeZones();
    addrGroup->exclude_zones = addressRange.excludeZones();

    addrGroup->include_is_empty = addressRange.includeRange().isEmpty();
    addrGroup->exclude_is_empty = addressRange.excludeRange().isEmpty();

    m_data += FORT_CONF_ADDR_GROUP_OFF;

    writeAddressList(addressRange.includeRange());

    addrGroup->exclude_off = m_data - addrGroup->data;

    writeAddressList(addressRange.excludeRange());
}

void ConfData::writeAddressList(const IpRange &ipRange)
{
    writeIpRange(ipRange);
    writeIpRange(ipRange, /*isIPv6=*/true);
}

void ConfData::writeIpRange(const IpRange &ipRange, bool isIPv6)
{
    PFORT_CONF_ADDR_LIST addrList = PFORT_CONF_ADDR_LIST(m_data);

    addrList->ip_n = quint32(isIPv6 ? ipRange.ip6Size() : ipRange.ip4Size());
    addrList->pair_n = quint32(isIPv6 ? ipRange.pair6Size() : ipRange.pair4Size());

    m_data += FORT_CONF_ADDR_LIST_OFF;

    if (isIPv6) {
        writeIp6Array(ipRange.ip6Array());
        writeIp6Array(ipRange.pair6FromArray());
        writeIp6Array(ipRange.pair6ToArray());
    } else {
        writeLongs(ipRange.ip4Array());
        writeLongs(ipRange.pair4FromArray());
        writeLongs(ipRange.pair4ToArray());
    }
}

void ConfData::writeApps(const appdata_map_t &appsMap, bool useHeader)
{
    quint32 *offp = (quint32 *) m_data;
    const quint32 offTableSize = quint32(useHeader ? FORT_CONF_STR_HEADER_SIZE(appsMap.size()) : 0);
    char *p = m_data + offTableSize;
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

        PFORT_APP_ENTRY entry = PFORT_APP_ENTRY(p);
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

    m_data += offTableSize + FORT_CONF_STR_DATA_SIZE(off);
}

void ConfData::migrateZoneData(const QByteArray &zoneData)
{
    PFORT_CONF_ADDR_LIST addr_list = PFORT_CONF_ADDR_LIST(zoneData.data());

    if (FORT_CONF_ADDR4_LIST_SIZE(addr_list->ip_n, addr_list->pair_n) == zoneData.size()) {
        IpRange ipRange;
        writeIpRange(ipRange, /*isIPv6=*/true);
    }
}

void ConfData::writeBytes(const bytes_arr_t &array)
{
    writeData(array.constData(), array.size(), sizeof(quint8));
}

void ConfData::writeShorts(const shorts_arr_t &array)
{
    writeData(array.constData(), array.size(), sizeof(quint16));
}

void ConfData::writeLongs(const longs_arr_t &array)
{
    writeData(array.constData(), array.size(), sizeof(quint32));
}

void ConfData::writeIp6Array(const ip6_arr_t &array)
{
    writeData(array.constData(), array.size(), sizeof(ip6_addr_t));
}

void ConfData::writeData(void const *src, int elemCount, uint elemSize)
{
    const size_t arraySize = size_t(elemCount) * elemSize;

    memcpy(m_data, src, arraySize);

    m_data += arraySize;
}

void ConfData::writeChars(const chars_arr_t &array)
{
    const size_t arraySize = size_t(array.size());

    memcpy(m_data, array.constData(), arraySize);

    m_data += FORT_CONF_STR_DATA_SIZE(arraySize);
}

void ConfData::writeArray(const QByteArray &array)
{
    const size_t arraySize = size_t(array.size());

    memcpy(m_data, array.constData(), arraySize);

    m_data += arraySize;
}

void ConfData::writeString(const QString &s)
{
    wchar_t *array = (wchar_t *) m_data;

    const int n = s.toWCharArray(array);

    array[n] = L'\0';

    m_data += (n + 1) * sizeof(wchar_t); // +1 for the null terminator
}
