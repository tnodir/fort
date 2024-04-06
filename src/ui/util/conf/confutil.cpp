#include "confutil.h"

#include <common/fortconf.h>
#include <fort_version.h>

#include <conf/addressgroup.h>
#include <conf/app.h>
#include <conf/appgroup.h>
#include <conf/firewallconf.h>
#include <driver/drivercommon.h>
#include <manager/envmanager.h>
#include <util/bitutil.h>
#include <util/dateutil.h>
#include <util/fileutil.h>
#include <util/stringutil.h>

#include "confappswalker.h"

#define APP_GROUP_MAX      FORT_CONF_GROUP_MAX
#define APP_GROUP_NAME_MAX 128
#define APP_PATH_MAX       FORT_CONF_APP_PATH_MAX

namespace {

inline bool checkIpRangeSize(const IpRange &range)
{
    return (range.ip4Size() + range.pair4Size()) < FORT_CONF_IP_MAX
            && (range.ip6Size() + range.pair6Size()) < FORT_CONF_IP_MAX;
}

int writeServicesHeader(char *data, int servicesCount)
{
    PFORT_SERVICE_INFO_LIST infoList = (PFORT_SERVICE_INFO_LIST) data;

    infoList->services_n = servicesCount;

    return FORT_SERVICE_INFO_LIST_DATA_OFF;
}

int writeServiceInfo(char *data, const ServiceInfo &serviceInfo)
{
    PFORT_SERVICE_INFO info = (PFORT_SERVICE_INFO) data;

    info->process_id = serviceInfo.processId;

    const quint16 nameLen = quint16(serviceInfo.serviceName.size() * sizeof(char16_t));
    info->name_len = nameLen;

    memcpy(info->name, serviceInfo.serviceName.utf16(), nameLen);

    return FORT_SERVICE_INFO_NAME_OFF + FORT_CONF_STR_DATA_SIZE(nameLen);
}

void writeConfFlags(const FirewallConf &conf, PFORT_CONF_FLAGS confFlags)
{
    confFlags->boot_filter = conf.bootFilter();
    confFlags->filter_enabled = conf.filterEnabled();
    confFlags->filter_locals = conf.filterLocals();
    confFlags->block_traffic = conf.blockTraffic();
    confFlags->block_inet_traffic = conf.blockInetTraffic();
    confFlags->allow_all_new = conf.allowAllNew();
    confFlags->ask_to_connect = conf.askToConnect();

    confFlags->app_block_all = conf.appBlockAll();
    confFlags->app_allow_all = conf.appAllowAll();

    confFlags->log_stat = true; // always enabled for driver
    confFlags->log_stat_no_filter = conf.logStatNoFilter();
    confFlags->log_blocked = conf.logBlocked();

    confFlags->log_allowed_ip = conf.logAllowedIp();
    confFlags->log_blocked_ip = conf.logBlockedIp();
    confFlags->log_alerted_blocked_ip = conf.logAlertedBlockedIp();

    confFlags->group_bits = conf.appGroupBits();
}

}

ConfUtil::ConfUtil(QObject *parent) : QObject(parent) { }

int ConfUtil::ruleMaxCount()
{
    return FORT_CONF_RULE_MAX;
}

int ConfUtil::zoneMaxCount()
{
    return FORT_CONF_ZONE_MAX;
}

int ConfUtil::writeVersion(QByteArray &buf)
{
    const int verSize = sizeof(FORT_CONF_VERSION);

    buf.reserve(verSize);

    // Fill the buffer
    PFORT_CONF_VERSION confVer = (PFORT_CONF_VERSION) buf.data();

    confVer->driver_version = DRIVER_VERSION;

    return verSize;
}

int ConfUtil::writeServices(
        const QVector<ServiceInfo> &services, int runningServicesCount, QByteArray &buf)
{
    buf.reserve(FORT_SERVICE_INFO_LIST_MIN_SIZE);

    int outSize = writeServicesHeader(buf.data(), runningServicesCount);

    for (const ServiceInfo &info : services) {
        if (!info.isRunning)
            continue;

        buf.reserve(outSize + FORT_SERVICE_INFO_MAX_SIZE);

        outSize += writeServiceInfo(buf.data() + outSize, info);
    }

    return outSize;
}

int ConfUtil::write(const FirewallConf &conf, ConfAppsWalker *confAppsWalker,
        EnvManager &envManager, QByteArray &buf)
{
    quint32 addressGroupsSize = 0;
    longs_arr_t addressGroupOffsets;
    addrranges_arr_t addressRanges(conf.addressGroups().size());

    if (!parseAddressGroups(
                conf.addressGroups(), addressRanges, addressGroupOffsets, addressGroupsSize))
        return 0;

    quint8 appPeriodsCount = 0;
    chars_arr_t appPeriods;

    AppParseOptions opt;

    if (!parseExeApps(envManager, confAppsWalker, opt))
        return 0;

    if (!parseAppGroups(envManager, conf.appGroups(), appPeriods, appPeriodsCount, opt))
        return 0;

    const quint32 appsSize = opt.wildAppsSize + opt.prefixAppsSize + opt.exeAppsSize;
    if (appsSize > FORT_CONF_APPS_LEN_MAX) {
        setErrorMessage(tr("Too many application paths"));
        return 0;
    }

    // Fill the buffer
    const int confIoSize = int(FORT_CONF_IO_CONF_OFF + FORT_CONF_DATA_OFF + addressGroupsSize
            + FORT_CONF_STR_DATA_SIZE(conf.appGroups().size() * sizeof(FORT_PERIOD)) // appPeriods
            + FORT_CONF_STR_DATA_SIZE(opt.wildAppsSize)
            + FORT_CONF_STR_HEADER_SIZE(opt.prefixAppsMap.size())
            + FORT_CONF_STR_DATA_SIZE(opt.prefixAppsSize)
            + FORT_CONF_STR_DATA_SIZE(opt.exeAppsSize));

    buf.reserve(confIoSize);

    writeConf(
            buf.data(), conf, addressRanges, addressGroupOffsets, appPeriods, appPeriodsCount, opt);

    return confIoSize;
}

int ConfUtil::writeFlags(const FirewallConf &conf, QByteArray &buf)
{
    const int flagsSize = sizeof(FORT_CONF_FLAGS);

    buf.reserve(flagsSize);

    // Fill the buffer
    PFORT_CONF_FLAGS confFlags = (PFORT_CONF_FLAGS) buf.data();

    writeConfFlags(conf, confFlags);

    return flagsSize;
}

int ConfUtil::writeAppEntry(const App &app, bool isNew, QByteArray &buf)
{
    appdata_map_t appsMap;
    quint32 appsSize = 0;

    if (!addApp(app, isNew, appsMap, appsSize))
        return 0;

    buf.reserve(appsSize);

    // Fill the buffer
    char *data = (char *) buf.data();

    writeApps(&data, appsMap);

    return int(appsSize);
}

int ConfUtil::writeZone(const IpRange &ipRange, QByteArray &buf)
{
    const int addrSize = FORT_CONF_ADDR_LIST_SIZE(
            ipRange.ip4Size(), ipRange.pair4Size(), ipRange.ip6Size(), ipRange.pair6Size());

    buf.reserve(addrSize);

    // Fill the buffer
    char *data = (char *) buf.data();

    writeAddressList(&data, ipRange);

    return addrSize;
}

int ConfUtil::writeZones(quint32 zonesMask, quint32 enabledMask, quint32 dataSize,
        const QList<QByteArray> &zonesData, QByteArray &buf)
{
    const int zonesSize = FORT_CONF_ZONES_DATA_OFF + dataSize;

    buf.reserve(zonesSize);

    // Fill the buffer
    PFORT_CONF_ZONES confZones = (PFORT_CONF_ZONES) buf.data();
    char *data = confZones->data;

    memset(confZones, 0, sizeof(FORT_CONF_ZONES_DATA_OFF));

    confZones->mask = zonesMask;
    confZones->enabled_mask = enabledMask;

    for (const auto &zoneData : zonesData) {
        Q_ASSERT(!zoneData.isEmpty());

        const int zoneIndex = BitUtil::bitScanForward(zonesMask);
        const quint32 zoneMask = (quint32(1) << zoneIndex);

#define CONF_DATA_OFFSET quint32(data - confZones->data)
        confZones->addr_off[zoneIndex] = CONF_DATA_OFFSET;
        writeArray(&data, zoneData);
        migrateZoneData(&data, zoneData);
#undef CONF_DATA_OFFSET

        zonesMask ^= zoneMask;
    }

    return zonesSize;
}

void ConfUtil::migrateZoneData(char **data, const QByteArray &zoneData)
{
    PFORT_CONF_ADDR4_LIST addr_list = (PFORT_CONF_ADDR4_LIST) zoneData.data();

    if (FORT_CONF_ADDR4_LIST_SIZE(addr_list->ip_n, addr_list->pair_n) == zoneData.size()) {
        IpRange ipRange;
        writeAddress6List(data, ipRange);
    }
}

int ConfUtil::writeZoneFlag(int zoneId, bool enabled, QByteArray &buf)
{
    const int flagSize = sizeof(FORT_CONF_ZONE_FLAG);

    buf.reserve(flagSize);

    // Fill the buffer
    PFORT_CONF_ZONE_FLAG confZoneFlag = (PFORT_CONF_ZONE_FLAG) buf.data();

    confZoneFlag->zone_id = zoneId;
    confZoneFlag->enabled = enabled;

    return flagSize;
}

bool ConfUtil::loadZone(const QByteArray &buf, IpRange &ipRange)
{
    const char *data = buf.data();
    uint bufSize = buf.size();

    return loadAddressList(&data, ipRange, bufSize);
}

bool ConfUtil::parseAddressGroups(const QList<AddressGroup *> &addressGroups,
        addrranges_arr_t &addressRanges, longs_arr_t &addressGroupOffsets,
        quint32 &addressGroupsSize)
{
    const int groupsCount = addressGroups.size();

    addressGroupsSize = quint32(groupsCount) * sizeof(quint32); // offsets

    for (int i = 0; i < groupsCount; ++i) {
        AddressGroup *addressGroup = addressGroups.at(i);

        AddressRange &addressRange = addressRanges[i];
        addressRange.setIncludeAll(addressGroup->includeAll());
        addressRange.setExcludeAll(addressGroup->excludeAll());
        addressRange.setIncludeZones(addressGroup->includeZones());
        addressRange.setExcludeZones(addressGroup->excludeZones());

        if (!addressRange.includeRange().fromText(addressGroup->includeText())) {
            setErrorMessage(
                    tr("Bad Include IP address: #%1 %2")
                            .arg(QString::number(i),
                                    addressRange.includeRange().errorLineAndMessageDetails()));
            return false;
        }

        if (!addressRange.excludeRange().fromText(addressGroup->excludeText())) {
            setErrorMessage(
                    tr("Bad Exclude IP address: #%1 %2")
                            .arg(QString::number(i),
                                    addressRange.excludeRange().errorLineAndMessageDetails()));
            return false;
        }

        const IpRange &incRange = addressRange.includeRange();
        const IpRange &excRange = addressRange.excludeRange();

        if (!(checkIpRangeSize(incRange) && checkIpRangeSize(excRange))) {
            setErrorMessage(tr("Too many IP addresses"));
            return false;
        }

        addressGroupOffsets.append(addressGroupsSize);

        addressGroupsSize += FORT_CONF_ADDR_GROUP_OFF
                + FORT_CONF_ADDR_LIST_SIZE(incRange.ip4Size(), incRange.pair4Size(),
                        incRange.ip6Size(), incRange.pair6Size())
                + FORT_CONF_ADDR_LIST_SIZE(excRange.ip4Size(), excRange.pair4Size(),
                        excRange.ip6Size(), excRange.pair6Size());
    }

    return true;
}

bool ConfUtil::parseAppGroups(EnvManager &envManager, const QList<AppGroup *> &appGroups,
        chars_arr_t &appPeriods, quint8 &appPeriodsCount, AppParseOptions &opt)
{
    const int groupsCount = appGroups.size();
    if (groupsCount < 1 || groupsCount > APP_GROUP_MAX) {
        setErrorMessage(
                tr("Number of Application Groups must be between 1 and %1").arg(APP_GROUP_MAX));
        return false;
    }

    for (int i = 0; i < groupsCount; ++i) {
        const AppGroup *appGroup = appGroups.at(i);

        const QString name = appGroup->name();
        if (name.size() > APP_GROUP_NAME_MAX) {
            setErrorMessage(
                    tr("Length of Application Group's Name must be < %1").arg(APP_GROUP_NAME_MAX));
            return false;
        }

        App app;
        app.applyChild = appGroup->applyChild();
        app.lanOnly = appGroup->lanOnly();
        app.logBlocked = appGroup->logBlocked();
        app.logConn = appGroup->logConn();
        app.groupIndex = i;

        app.appOriginPath = appGroup->killText();
        app.blocked = true;
        app.killProcess = true;
        if (!parseAppsText(envManager, app, opt))
            return false;

        app.appOriginPath = appGroup->blockText();
        app.blocked = true;
        app.killProcess = false;
        if (!parseAppsText(envManager, app, opt))
            return false;

        app.appOriginPath = appGroup->allowText();
        app.blocked = false;
        if (!parseAppsText(envManager, app, opt))
            return false;

        // Enabled Period
        parseAppPeriod(appGroup, appPeriods, appPeriodsCount);
    }

    return true;
}

bool ConfUtil::parseExeApps(
        EnvManager &envManager, ConfAppsWalker *confAppsWalker, AppParseOptions &opt)
{
    if (Q_UNLIKELY(!confAppsWalker))
        return true;

    return confAppsWalker->walkApps([&](App &app) -> bool {
        if (app.isWildcard) {
            return parseAppsText(envManager, app, opt);
        } else {
            return addApp(app, /*isNew=*/true, opt.exeAppsMap, opt.exeAppsSize);
        }
    });
}

bool ConfUtil::parseAppsText(EnvManager &envManager, App &app, AppParseOptions &opt)
{
    const auto text = envManager.expandString(app.appOriginPath);
    const auto lines = StringUtil::tokenizeView(text, QLatin1Char('\n'));

    for (const auto &line : lines) {
        const auto lineTrimmed = line.trimmed();
        if (lineTrimmed.isEmpty() || lineTrimmed.startsWith('#')) // commented line
            continue;

        if (!parseAppLine(app, lineTrimmed, opt))
            return false;
    }

    return true;
}

bool ConfUtil::parseAppLine(App &app, const QStringView &line, AppParseOptions &opt)
{
    bool isWild = false;
    bool isPrefix = false;
    const QString appPath = parseAppPath(line, isWild, isPrefix);
    if (appPath.isEmpty())
        return true;

    app.appPath = appPath;
    app.useGroupPerm = true;
    app.alerted = false;

    if (isWild || isPrefix) {
        if (app.isProcWild()) {
            opt.procWild = true;
        }
    }

    appdata_map_t &appsMap = opt.appsMap(isWild, isPrefix);
    quint32 &appsSize = opt.appsSize(isWild, isPrefix);

    return addApp(app, /*isNew=*/true, appsMap, appsSize);
}

bool ConfUtil::addApp(const App &app, bool isNew, appdata_map_t &appsMap, quint32 &appsSize)
{
    const QString kernelPath = FileUtil::pathToKernelPath(app.appPath);

    if (appsMap.contains(kernelPath))
        return true;

    const int kernelPathSize = kernelPath.size();

    if (kernelPathSize > APP_PATH_MAX) {
        setErrorMessage(tr("Length of Application's Path must be < %1").arg(APP_PATH_MAX));
        return false;
    }

    const quint16 appPathLen = quint16(kernelPathSize * sizeof(wchar_t));
    const quint32 appSize = FORT_CONF_APP_ENTRY_SIZE(appPathLen);

    appsSize += appSize;

    const FORT_APP_DATA appData = {
        .flags = {
                .group_index = quint8(app.groupIndex),
                .use_group_perm = app.useGroupPerm,
                .apply_child = app.applyChild,
                .kill_child = app.killChild,
                .lan_only = app.lanOnly,
                .log_blocked = app.logBlocked,
                .log_conn = app.logConn,
                .blocked = app.blocked,
                .kill_process = app.killProcess,
                .alerted = app.alerted,
                .is_new = isNew,
                .found = 1,
        },
        .rule_id = app.ruleId,
        .accept_zones = quint16(app.acceptZones),
        .reject_zones = quint16(app.rejectZones),
    };

    appsMap.insert(kernelPath, appData);

    m_driveMask |= FileUtil::driveMaskByPath(app.appPath);

    return true;
}

QString ConfUtil::parseAppPath(const QStringView line, bool &isWild, bool &isPrefix)
{
    static const QRegularExpression wildMatcher("([*?[])");

    auto path = line;
    if (path.startsWith('"') && path.endsWith('"')) {
        path = path.mid(1, path.size() - 2);
    }

    if (path.isEmpty())
        return QString();

    const auto wildMatch = StringUtil::match(wildMatcher, path);
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

void ConfUtil::parseAppPeriod(
        const AppGroup *appGroup, chars_arr_t &appPeriods, quint8 &appPeriodsCount)
{
    quint8 fromHour = 0, fromMinute = 0;
    quint8 toHour = 0, toMinute = 0;

    if (appGroup->periodEnabled()) {
        DateUtil::parseTime(appGroup->periodFrom(), fromHour, fromMinute);
        DateUtil::parseTime(appGroup->periodTo(), toHour, toMinute);

        const bool fromIsEmpty = (fromHour == 0 && fromMinute == 0);
        const bool toIsEmpty = (toHour == 0 && toMinute == 0);

        if (!fromIsEmpty || !toIsEmpty) {
            ++appPeriodsCount;
        }
    }

    appPeriods.append(qint8(fromHour));
    appPeriods.append(qint8(fromMinute));
    appPeriods.append(qint8(toHour));
    appPeriods.append(qint8(toMinute));
}

void ConfUtil::writeConf(char *output, const FirewallConf &conf,
        const addrranges_arr_t &addressRanges, const longs_arr_t &addressGroupOffsets,
        const chars_arr_t &appPeriods, quint8 appPeriodsCount, AppParseOptions &opt)
{
    PFORT_CONF_IO drvConfIo = (PFORT_CONF_IO) output;
    PFORT_CONF drvConf = &drvConfIo->conf;
    char *data = drvConf->data;
    quint32 addrGroupsOff;
    quint32 appPeriodsOff;
    quint32 wildAppsOff, prefixAppsOff, exeAppsOff;

#define CONF_DATA_OFFSET quint32(data - drvConf->data)
    addrGroupsOff = CONF_DATA_OFFSET;
    writeLongs(&data, addressGroupOffsets);
    writeAddressRanges(&data, addressRanges);

    appPeriodsOff = CONF_DATA_OFFSET;
    writeChars(&data, appPeriods);

    wildAppsOff = CONF_DATA_OFFSET;
    writeApps(&data, opt.wildAppsMap);

    prefixAppsOff = CONF_DATA_OFFSET;
    writeApps(&data, opt.prefixAppsMap, /*useHeader=*/true);

    exeAppsOff = CONF_DATA_OFFSET;
    writeApps(&data, opt.exeAppsMap);
#undef CONF_DATA_OFFSET

    writeAppGroupFlags(&drvConfIo->conf_group.group_bits, &drvConfIo->conf_group.log_blocked,
            &drvConfIo->conf_group.log_conn, conf);

    writeLimits(drvConfIo->conf_group.limits, &drvConfIo->conf_group.limit_bits,
            &drvConfIo->conf_group.limit_io_bits, conf.appGroups());

    writeConfFlags(conf, &drvConf->flags);

    DriverCommon::confAppPermsMaskInit(drvConf);

    drvConf->app_periods_n = appPeriodsCount;

    drvConf->proc_wild = opt.procWild;

    drvConf->wild_apps_n = quint16(opt.wildAppsMap.size());
    drvConf->prefix_apps_n = quint16(opt.prefixAppsMap.size());
    drvConf->exe_apps_n = quint16(opt.exeAppsMap.size());

    drvConf->addr_groups_off = addrGroupsOff;

    drvConf->app_periods_off = appPeriodsOff;

    drvConf->wild_apps_off = wildAppsOff;
    drvConf->prefix_apps_off = prefixAppsOff;
    drvConf->exe_apps_off = exeAppsOff;
}

void ConfUtil::writeAppGroupFlags(
        quint16 *groupBits, quint16 *logBlockedBits, quint16 *logConnBits, const FirewallConf &conf)
{
    *groupBits = 0;
    *logBlockedBits = 0;
    *logConnBits = 0;

    int i = 0;
    for (const AppGroup *appGroup : conf.appGroups()) {
        if (appGroup->enabled()) {
            *groupBits |= (1 << i);
        }
        if (appGroup->logBlocked()) {
            *logBlockedBits |= (1 << i);
        }
        if (appGroup->logConn()) {
            *logConnBits |= (1 << i);
        }
        ++i;
    }
}

void ConfUtil::writeLimits(struct fort_speed_limit *limits, quint16 *limitBits,
        quint32 *limitIoBits, const QList<AppGroup *> &appGroups)
{
    *limitBits = 0;
    *limitIoBits = 0;

    const int groupsCount = appGroups.size();
    for (int i = 0; i < groupsCount; ++i, limits += 2) {
        const AppGroup *appGroup = appGroups.at(i);

        const quint32 limitIn = appGroup->enabledSpeedLimitIn();
        const quint32 limitOut = appGroup->enabledSpeedLimitOut();

        const bool isLimitIn = (limitIn != 0);
        const bool isLimitOut = (limitOut != 0);

        if (isLimitIn || isLimitOut) {
            *limitBits |= (1 << i);

            if (isLimitIn) {
                *limitIoBits |= (1 << (i * 2 + 0));

                writeLimit(&limits[0], limitIn, appGroup->limitBufferSizeIn(),
                        appGroup->limitLatency(), appGroup->limitPacketLoss());
            }

            if (isLimitOut) {
                *limitIoBits |= (1 << (i * 2 + 1));

                writeLimit(&limits[1], limitOut, appGroup->limitBufferSizeOut(),
                        appGroup->limitLatency(), appGroup->limitPacketLoss());
            }
        }
    }
}

void ConfUtil::writeLimit(fort_speed_limit *limit, quint32 kBits, quint32 bufferSize,
        quint32 latencyMsec, quint16 packetLoss)
{
    limit->plr = packetLoss;
    limit->latency_ms = latencyMsec;
    limit->buffer_bytes = bufferSize;
    limit->bps = quint64(kBits) * (1024LL / 8); /* to bytes per second */
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
    writeAddress4List(data, ipRange);
    writeAddress6List(data, ipRange);
}

void ConfUtil::writeAddress4List(char **data, const IpRange &ipRange)
{
    PFORT_CONF_ADDR4_LIST addrList = PFORT_CONF_ADDR4_LIST(*data);

    addrList->ip_n = quint32(ipRange.ip4Size());
    addrList->pair_n = quint32(ipRange.pair4Size());

    *data += FORT_CONF_ADDR4_LIST_OFF;

    writeLongs(data, ipRange.ip4Array());
    writeLongs(data, ipRange.pair4FromArray());
    writeLongs(data, ipRange.pair4ToArray());
}

void ConfUtil::writeAddress6List(char **data, const IpRange &ipRange)
{
    PFORT_CONF_ADDR6_LIST addrList = PFORT_CONF_ADDR6_LIST(*data);

    addrList->ip_n = quint32(ipRange.ip6Size());
    addrList->pair_n = quint32(ipRange.pair6Size());

    *data += FORT_CONF_ADDR6_LIST_OFF;

    writeIp6Array(data, ipRange.ip6Array());
    writeIp6Array(data, ipRange.pair6FromArray());
    writeIp6Array(data, ipRange.pair6ToArray());
}

bool ConfUtil::loadAddressList(const char **data, IpRange &ipRange, uint &bufSize)
{
    return loadAddress4List(data, ipRange, bufSize)
            && (bufSize == 0 || loadAddress6List(data, ipRange, bufSize));
}

bool ConfUtil::loadAddress4List(const char **data, IpRange &ipRange, uint &bufSize)
{
    if (bufSize < FORT_CONF_ADDR4_LIST_OFF)
        return false;

    PFORT_CONF_ADDR4_LIST addr_list = (PFORT_CONF_ADDR4_LIST) *data;
    *data = (const char *) addr_list->ip;

    const uint addrListSize = FORT_CONF_ADDR4_LIST_SIZE(addr_list->ip_n, addr_list->pair_n);
    if (bufSize < addrListSize)
        return false;

    bufSize -= addrListSize;

    ipRange.ip4Array().resize(addr_list->ip_n);
    ipRange.pair4FromArray().resize(addr_list->pair_n);
    ipRange.pair4ToArray().resize(addr_list->pair_n);

    loadLongs(data, ipRange.ip4Array());
    loadLongs(data, ipRange.pair4FromArray());
    loadLongs(data, ipRange.pair4ToArray());

    return true;
}

bool ConfUtil::loadAddress6List(const char **data, IpRange &ipRange, uint &bufSize)
{
    if (bufSize < FORT_CONF_ADDR6_LIST_OFF)
        return false;

    PFORT_CONF_ADDR6_LIST addr_list = (PFORT_CONF_ADDR6_LIST) *data;
    *data = (const char *) addr_list->ip;

    const uint addrListSize = FORT_CONF_ADDR6_LIST_SIZE(addr_list->ip_n, addr_list->pair_n);
    if (bufSize < addrListSize)
        return false;

    bufSize -= addrListSize;

    ipRange.ip6Array().resize(addr_list->ip_n);
    ipRange.pair6FromArray().resize(addr_list->pair_n);
    ipRange.pair6ToArray().resize(addr_list->pair_n);

    loadIp6Array(data, ipRange.ip6Array());
    loadIp6Array(data, ipRange.pair6FromArray());
    loadIp6Array(data, ipRange.pair6ToArray());

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
