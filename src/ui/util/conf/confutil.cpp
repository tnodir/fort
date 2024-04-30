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
#include "confruleswalker.h"

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

ConfUtil::ConfUtil(const QByteArray &buffer, QObject *parent) :
    QObject(parent), m_buffer(buffer) { }

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

void ConfUtil::writeVersion()
{
    const int verSize = sizeof(FORT_CONF_VERSION);

    buffer().resize(verSize);

    // Fill the buffer
    PFORT_CONF_VERSION confVer = (PFORT_CONF_VERSION) buffer().data();

    confVer->driver_version = DRIVER_VERSION;
}

void ConfUtil::writeServices(const QVector<ServiceInfo> &services, int runningServicesCount)
{
    const int servicesSize =
            FORT_SERVICE_INFO_LIST_MIN_SIZE + runningServicesCount * FORT_SERVICE_INFO_MAX_SIZE;

    buffer().resize(servicesSize);

    char *data = buffer().data();

    int outSize = writeServicesHeader(data, runningServicesCount);

    for (const ServiceInfo &info : services) {
        if (!info.isRunning)
            continue;

        outSize += writeServiceInfo(data + outSize, info);
    }

    buffer().resize(outSize);
}

bool ConfUtil::write(
        const FirewallConf &conf, const ConfAppsWalker *confAppsWalker, EnvManager &envManager)
{
    WriteConfArgs wca = { .conf = conf,
        .ad = { .addressRanges = addrranges_arr_t(conf.addressGroups().size()) } };

    quint32 addressGroupsSize = 0;

    if (!parseAddressGroups(conf.addressGroups(), wca.ad, addressGroupsSize))
        return false;

    AppParseOptions opt;

    if (!parseExeApps(envManager, confAppsWalker, opt))
        return false;

    if (!parseAppGroups(envManager, conf.appGroups(), wca.gr, opt))
        return false;

    const quint32 appsSize = opt.wildAppsSize + opt.prefixAppsSize + opt.exeAppsSize;
    if (appsSize > FORT_CONF_APPS_LEN_MAX) {
        setErrorMessage(tr("Too many application paths"));
        return false;
    }

    // Fill the buffer
    const int confIoSize = int(FORT_CONF_IO_CONF_OFF + FORT_CONF_DATA_OFF + addressGroupsSize
            + FORT_CONF_STR_DATA_SIZE(conf.appGroups().size() * sizeof(FORT_PERIOD)) // appPeriods
            + FORT_CONF_STR_DATA_SIZE(opt.wildAppsSize)
            + FORT_CONF_STR_HEADER_SIZE(opt.prefixAppsMap.size())
            + FORT_CONF_STR_DATA_SIZE(opt.prefixAppsSize)
            + FORT_CONF_STR_DATA_SIZE(opt.exeAppsSize));

    buffer().resize(confIoSize);

    writeConf(buffer().data(), wca, opt);

    return true;
}

void ConfUtil::writeFlags(const FirewallConf &conf)
{
    const int flagsSize = sizeof(FORT_CONF_FLAGS);

    buffer().resize(flagsSize);

    // Fill the buffer
    PFORT_CONF_FLAGS confFlags = (PFORT_CONF_FLAGS) buffer().data();

    writeConfFlags(conf, confFlags);
}

bool ConfUtil::writeAppEntry(const App &app, bool isNew)
{
    appdata_map_t appsMap;
    quint32 appsSize = 0;

    if (!addApp(app, isNew, appsMap, appsSize))
        return false;

    buffer().resize(appsSize);

    // Fill the buffer
    char *data = buffer().data();

    writeApps(&data, appsMap);

    return true;
}

bool ConfUtil::writeRules(const ConfRulesWalker &confRulesWalker)
{
    ruleset_map_t ruleSetMap;
    ruleid_arr_t ruleIds;
    int maxRuleId;

    int outSize = 0;
    int ruleOffset = 0;

    return confRulesWalker.walkRules(ruleSetMap, ruleIds, maxRuleId, [&](Rule &rule) -> bool {
        if (outSize == 0) {
            outSize = FORT_CONF_RULES_DATA_OFF + FORT_CONF_RULES_OFFSETS_SIZE(maxRuleId);

            buffer().resize(outSize);
            buffer().fill('\0');

            PFORT_CONF_RULES rules = (PFORT_CONF_RULES) buffer().data();
            rules->max_rule_id = maxRuleId;
        }

        const int ruleId = rule.ruleId;
        const auto ruleSetIndex = ruleSetMap[ruleId];

        // Store the rule's offset
        {
            int *ruleOffsets = (int *) (buffer().data() + FORT_CONF_RULES_DATA_OFF);
            ruleOffsets[ruleId] = ruleOffset;
        }

        // Store the rule
        PFORT_CONF_RULE confRule = (PFORT_CONF_RULE) (buffer().data() + outSize);
        confRule->enabled = rule.enabled;
        confRule->blocked = rule.blocked;
        confRule->exclusive = rule.exclusive;

        const bool hasZones = (rule.acceptZones != 0 || rule.rejectZones != 0);
        confRule->has_zones = hasZones;

        confRule->has_expr = !rule.ruleText.isEmpty();

        confRule->set_count = ruleSetIndex.count;

        return true;
    });
}

void ConfUtil::writeZone(const IpRange &ipRange)
{
    const int addrSize = FORT_CONF_ADDR_LIST_SIZE(
            ipRange.ip4Size(), ipRange.pair4Size(), ipRange.ip6Size(), ipRange.pair6Size());

    buffer().resize(addrSize);

    // Fill the buffer
    char *data = buffer().data();

    writeAddressList(&data, ipRange);
}

void ConfUtil::writeZones(quint32 zonesMask, quint32 enabledMask, quint32 dataSize,
        const QList<QByteArray> &zonesData)
{
    const int zonesSize = FORT_CONF_ZONES_DATA_OFF + dataSize;

    buffer().resize(zonesSize);

    // Fill the buffer
    PFORT_CONF_ZONES confZones = (PFORT_CONF_ZONES) buffer().data();
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
}

void ConfUtil::migrateZoneData(char **data, const QByteArray &zoneData)
{
    PFORT_CONF_ADDR4_LIST addr_list = (PFORT_CONF_ADDR4_LIST) zoneData.data();

    if (FORT_CONF_ADDR4_LIST_SIZE(addr_list->ip_n, addr_list->pair_n) == zoneData.size()) {
        IpRange ipRange;
        writeAddress6List(data, ipRange);
    }
}

void ConfUtil::writeZoneFlag(int zoneId, bool enabled)
{
    const int flagSize = sizeof(FORT_CONF_ZONE_FLAG);

    buffer().resize(flagSize);

    // Fill the buffer
    PFORT_CONF_ZONE_FLAG confZoneFlag = (PFORT_CONF_ZONE_FLAG) buffer().data();

    confZoneFlag->zone_id = zoneId;
    confZoneFlag->enabled = enabled;
}

bool ConfUtil::loadZone(IpRange &ipRange)
{
    const char *data = buffer().data();
    uint bufSize = buffer().size();

    return loadAddressList(&data, ipRange, bufSize);
}

bool ConfUtil::parseAddressGroups(const QList<AddressGroup *> &addressGroups,
        ParseAddressGroupsArgs &ad, quint32 &addressGroupsSize)
{
    const int groupsCount = addressGroups.size();

    addressGroupsSize = quint32(groupsCount) * sizeof(quint32); // offsets

    for (int i = 0; i < groupsCount; ++i) {
        AddressGroup *addressGroup = addressGroups.at(i);

        AddressRange &addressRange = ad.addressRanges[i];
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

        ad.addressGroupOffsets.append(addressGroupsSize);

        addressGroupsSize += FORT_CONF_ADDR_GROUP_OFF
                + FORT_CONF_ADDR_LIST_SIZE(incRange.ip4Size(), incRange.pair4Size(),
                        incRange.ip6Size(), incRange.pair6Size())
                + FORT_CONF_ADDR_LIST_SIZE(excRange.ip4Size(), excRange.pair4Size(),
                        excRange.ip6Size(), excRange.pair6Size());
    }

    return true;
}

bool ConfUtil::parseAppGroups(EnvManager &envManager, const QList<AppGroup *> &appGroups,
        ParseAppGroupsArgs &gr, AppParseOptions &opt)
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
        parseAppPeriod(appGroup, gr);
    }

    return true;
}

bool ConfUtil::parseExeApps(
        EnvManager &envManager, const ConfAppsWalker *confAppsWalker, AppParseOptions &opt)
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

void ConfUtil::parseAppPeriod(const AppGroup *appGroup, ParseAppGroupsArgs &gr)
{
    quint8 fromHour = 0, fromMinute = 0;
    quint8 toHour = 0, toMinute = 0;

    if (appGroup->periodEnabled()) {
        DateUtil::parseTime(appGroup->periodFrom(), fromHour, fromMinute);
        DateUtil::parseTime(appGroup->periodTo(), toHour, toMinute);

        const bool fromIsEmpty = (fromHour == 0 && fromMinute == 0);
        const bool toIsEmpty = (toHour == 0 && toMinute == 0);

        if (!fromIsEmpty || !toIsEmpty) {
            ++gr.appPeriodsCount;
        }
    }

    gr.appPeriods.append(qint8(fromHour));
    gr.appPeriods.append(qint8(fromMinute));
    gr.appPeriods.append(qint8(toHour));
    gr.appPeriods.append(qint8(toMinute));
}

void ConfUtil::writeConf(char *output, const WriteConfArgs &wca, AppParseOptions &opt)
{
    PFORT_CONF_IO drvConfIo = (PFORT_CONF_IO) output;
    PFORT_CONF drvConf = &drvConfIo->conf;
    char *data = drvConf->data;
    quint32 addrGroupsOff;
    quint32 appPeriodsOff;
    quint32 wildAppsOff, prefixAppsOff, exeAppsOff;

#define CONF_DATA_OFFSET quint32(data - drvConf->data)
    addrGroupsOff = CONF_DATA_OFFSET;
    writeLongs(&data, wca.ad.addressGroupOffsets);
    writeAddressRanges(&data, wca.ad.addressRanges);

    appPeriodsOff = CONF_DATA_OFFSET;
    writeChars(&data, wca.gr.appPeriods);

    wildAppsOff = CONF_DATA_OFFSET;
    writeApps(&data, opt.wildAppsMap);

    prefixAppsOff = CONF_DATA_OFFSET;
    writeApps(&data, opt.prefixAppsMap, /*useHeader=*/true);

    exeAppsOff = CONF_DATA_OFFSET;
    writeApps(&data, opt.exeAppsMap);
#undef CONF_DATA_OFFSET

    writeAppGroupFlags(&drvConfIo->conf_group, wca.conf);

    writeLimits(&drvConfIo->conf_group, wca.conf.appGroups());

    writeConfFlags(wca.conf, &drvConf->flags);

    DriverCommon::confAppPermsMaskInit(drvConf);

    drvConf->app_periods_n = wca.gr.appPeriodsCount;

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
