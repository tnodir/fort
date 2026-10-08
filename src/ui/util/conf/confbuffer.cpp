#include "confbuffer.h"

#include <QHash>
#include <QMap>

#include <fort_version.h>

#include <conf/addressgroup.h>
#include <conf/app.h>
#include <conf/firewallconf.h>
#include <conf/group.h>
#include <conf/rule.h>
#include <conf/speedlimit.h>
#include <manager/envmanager.h>
#include <util/bitutil.h>
#include <util/fileutil.h>
#include <util/net/valuerangeutil.h>
#include <util/stringutil.h>

#include "confappswalker.h"
#include "confrodata.h"
#include "confruleswalker.h"
#include "confutil.h"
#include "ruletextparser.h"

namespace {

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

FORT_CONF_RULE confRuleFlags(const Rule &rule)
{
    FORT_CONF_RULE confRule {}; /* zero the reserved bits */
    confRule.enabled = rule.enabled;
    confRule.blocked = rule.blocked;
    confRule.exclusive = rule.exclusive;
    confRule.inline_zones = rule.inlineZones;
    confRule.terminate = rule.terminate;
    confRule.term_blocked = rule.terminateBlocked;
    confRule.term_alert = rule.terminateAlert;
    confRule.term_drop = rule.terminateDrop;
    confRule.log_allowed_conn = rule.logAllowedConn;
    confRule.log_blocked_conn = rule.logBlockedConn;

    confRule.has_zones = (rule.zones.accept_mask != 0 || rule.zones.reject_mask != 0);
    confRule.has_filters = !rule.ruleText.isEmpty();

    confRule.period_id = rule.periodEnabled ? rule.periodId : 0;

    return confRule;
}

}

ConfBuffer::ConfBuffer(const QByteArray &buffer, QObject *parent) :
    QObject(parent), m_buffer(buffer)
{
}

void ConfBuffer::writeVersion()
{
    // Resize the buffer
    const int verSize = sizeof(FORT_CONF_VERSION);

    buffer().resize(verSize);

    // Fill the buffer
    PFORT_CONF_VERSION confVer = (PFORT_CONF_VERSION) buffer().data();

    confVer->driver_version = DRIVER_VERSION;
}

void ConfBuffer::writeServices(const QVector<ServiceInfo> &services, int processCount)
{
    // Resize the buffer to max size
    const int servicesSize =
            FORT_SERVICE_INFO_LIST_MIN_SIZE + processCount * FORT_SERVICE_INFO_MAX_SIZE;

    buffer().resize(servicesSize);

    // Fill the buffer
    char *data = buffer().data();

    int outSize = writeServicesHeader(data, processCount);

    for (const ServiceInfo &info : services) {
        if (!info.hasProcess)
            continue;

        outSize += writeServiceInfo(data + outSize, info);
    }

    buffer().resize(outSize); // shrink to actual size
}

bool ConfBuffer::writeConf(const FirewallConf &conf, const ConfAppsWalker *confAppsWalker,
        EnvManager *envManager, quint64 activePeriodsMask)
{
    WriteConfArgs wca = {
        .conf = conf,
        .activePeriodsMask = activePeriodsMask,
        .ad = { .addressRanges = addrranges_arr_t(conf.addressGroups().size()) },
    };

    quint32 addressGroupsSize = 0;

    if (!parseAddressGroups(conf.addressGroups(), wca.ad, addressGroupsSize))
        return false;

    AppParseOptions opt;

    if (!parseExeApps(*envManager, confAppsWalker, opt))
        return false;

    const quint32 appsSize = opt.wildAppsSize + opt.prefixAppsSize + opt.exeAppsSize;
    if (appsSize > FORT_CONF_APPS_LEN_MAX) {
        setErrorMessage(tr("Too many application paths"));
        return false;
    }

    // Resize the buffer
    const int confIoSize = int(FORT_CONF_IO_CONF_OFF + FORT_CONF_DATA_OFF + addressGroupsSize
            + FORT_CONF_STR_DATA_SIZE(opt.wildAppsSize)
            + FORT_CONF_STR_HEADER_SIZE(opt.prefixAppsMap.size())
            + FORT_CONF_STR_DATA_SIZE(opt.prefixAppsSize)
            + FORT_CONF_STR_DATA_SIZE(opt.exeAppsSize));

    buffer().resize(confIoSize);

    // Fill the buffer
    char *data = buffer().data();

    ConfData(data).writeConf(wca, opt);

    return true;
}

void ConfBuffer::writeFlags(const FirewallConf &conf)
{
    // Resize the buffer
    const int flagsSize = sizeof(FORT_CONF_FLAGS);

    buffer().resize(flagsSize);

    // Fill the buffer
    char *data = buffer().data();

    ConfData(data).writeConfFlags(conf);
}

bool ConfBuffer::writeAppEntry(const App &app, bool isNew)
{
    appdata_map_t appsMap;
    quint32 appsSize = 0;

    if (!addApp(app, isNew, appsMap, appsSize))
        return false;

    // Resize the buffer
    buffer().resize(appsSize);

    // Fill the buffer
    char *data = buffer().data();

    ConfData(data).writeApps(appsMap);

    return true;
}

void ConfBuffer::writeZone(const IpRange &ipRange)
{
    // Resize the buffer
    const int addrSize = ipRange.sizeToWrite();

    buffer().resize(addrSize);

    // Fill the buffer
    char *data = buffer().data();

    ConfData(data).writeAddressList(ipRange);
}

void ConfBuffer::writeZones(quint32 zonesMask, quint32 enabledMask, quint32 dataSize,
        const QList<QByteArray> &zonesData)
{
    for (const auto &zoneData : zonesData) {
        dataSize += ConfData::migrateZoneDataSize(zoneData);
    }

    // Resize the buffer
    const int zonesSize = FORT_CONF_ZONES_DATA_OFF + dataSize;

    buffer().resize(zonesSize);

    // Fill the buffer
    char *data = buffer().data();

    PFORT_CONF_ZONES confZones = PFORT_CONF_ZONES(data);

    memset(confZones, 0, FORT_CONF_ZONES_DATA_OFF);

    confZones->mask = zonesMask;
    confZones->enabled_mask = enabledMask;

    data = confZones->data;

    ConfData confData(data);

    for (const auto &zoneData : zonesData) {
        Q_ASSERT(!zoneData.isEmpty());

        const int zoneIndex = BitUtil::bitScanForward(zonesMask);
        if (Q_UNLIKELY(zoneIndex == -1))
            break;

        const quint32 zoneMask = (quint32(1) << zoneIndex);

        confZones->addr_off[zoneIndex] = confData.dataOffset();

        confData.writeArray(zoneData);
        confData.migrateZoneData(zoneData);

        zonesMask ^= zoneMask;
    }
}

void ConfBuffer::writeZoneFlag(int zoneId, bool enabled)
{
    // Resize the buffer
    const int flagSize = sizeof(FORT_CONF_ZONE_FLAG);

    buffer().resize(flagSize);

    // Fill the buffer
    char *data = buffer().data();

    PFORT_CONF_ZONE_FLAG confZoneFlag = PFORT_CONF_ZONE_FLAG(data);

    confZoneFlag->zone_id = zoneId;
    confZoneFlag->enabled = enabled;
}

bool ConfBuffer::loadZone(IpRange &ipRange)
{
    const char *data = buffer().data();
    uint bufSize = buffer().size();

    return ConfRoData(data).loadAddressList(ipRange, bufSize);
}

bool ConfBuffer::parseAddressGroups(const QList<AddressGroup *> &addressGroups,
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
            setErrorMessage(tr("Bad Include IP address: #%1 %2")
                            .arg(QString::number(i),
                                    addressRange.includeRange().errorLineAndMessageDetails()));
            return false;
        }

        if (!addressRange.excludeRange().fromText(addressGroup->excludeText())) {
            setErrorMessage(tr("Bad Exclude IP address: #%1 %2")
                            .arg(QString::number(i),
                                    addressRange.excludeRange().errorLineAndMessageDetails()));
            return false;
        }

        const IpRange &incRange = addressRange.includeRange();
        const IpRange &excRange = addressRange.excludeRange();

        if (!(incRange.checkSize() && excRange.checkSize())) {
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

bool ConfBuffer::parseExeApps(
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

bool ConfBuffer::parseAppsText(EnvManager &envManager, App &app, AppParseOptions &opt)
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

bool ConfBuffer::parseAppLine(App &app, const QStringView line, AppParseOptions &opt)
{
    bool isWild = false;
    bool isPrefix = false;
    const QString appPath = ConfUtil::parseAppPath(line, isWild, isPrefix);
    if (appPath.isEmpty())
        return true;

    app.appPath = appPath;

    if (isWild || isPrefix) {
        if (app.isProcWild()) {
            opt.procWild = true;
        }
    }

    appdata_map_t &appsMap = opt.appsMap(isWild, isPrefix);
    quint32 &appsSize = opt.appsSize(isWild, isPrefix);

    return addApp(app, /*isNew=*/true, appsMap, appsSize);
}

bool ConfBuffer::addApp(const App &app, bool isNew, appdata_map_t &appsMap, quint32 &appsSize)
{
    const QString appPath = FileUtil::normalizePath(app.appPath);

    const auto it = appsMap.find(appPath);
    if (it != appsMap.end()) {
        it->appData.flags.has_wildcard_app |= app.isWildcard;
        return true;
    }

    const int appPathSize = appPath.size();

    if (appPathSize > FORT_CONF_APP_PATH_MAX) {
        setErrorMessage(
                tr("Length of Application's Path must be < %1").arg(FORT_CONF_APP_PATH_MAX));
        return false;
    }

    QByteArray ruleData;
    if (!parseAppRule(app, ruleData))
        return false;

    const quint16 appPathLen = quint16(appPathSize * sizeof(wchar_t));
    const quint32 appSize = FORT_CONF_APP_ENTRY_SIZE(appPathLen, quint32(ruleData.size()));

    appsSize += appSize;

    const FORT_APP_DATA appData = {
        .flags = {
                .apply_parent = app.applyParent,
                .apply_child = app.applyChild,
                .apply_spec_child = app.applySpecChild,
                .kill_child = app.killChild,
                .block_inbound = app.blockInbound,
                .block_outbound = app.blockOutbound,
                .lan_only = app.lanOnly,
                .log_stat = app.logStat,
                .log_allowed_conn = app.logAllowedConn,
                .log_blocked_conn = app.logBlockedConn,
                .blocked = app.blocked,
                .kill_process = app.killProcess,
                .is_new = isNew,
                .found = true,
                .has_wildcard_app = app.isWildcard,
        },
        .rule_id = app.ruleId,
        .speed_limits = app.activeSpeedLimits(),
        .groups = app.groups,
        .app_id = quint32(app.appId),
        .zones = app.zones,
    };

    appsMap.insert(appPath, { appData, ruleData });

    return true;
}

bool ConfBuffer::parseAppRule(const App &app, QByteArray &ruleData)
{
    if (app.filtersText.isEmpty())
        return true; // no Network Filters

    Rule rule;
    ConfUtil::parseAppFiltersText(app.filtersText, rule);

    if (rule.ruleText.isEmpty() && !rule.terminate)
        return true; // no Network Filters

    ConfBuffer ruleBuf;
    if (!ruleBuf.writeRuleData(rule)) {
        setErrorMessage(ruleBuf.errorMessage());
        return false;
    }

    ruleData = ruleBuf.buffer();

    return true;
}

bool ConfBuffer::writeRules(const ConfRulesWalker &confRulesWalker)
{
    WalkRulesArgs wra;

    return confRulesWalker.walkRules(wra, [&](const Rule &rule) -> bool {
        if (buffer().isEmpty()) {
            const int outSize =
                    FORT_CONF_RULES_DATA_OFF + FORT_CONF_RULES_OFFSETS_SIZE(wra.maxRuleId);

            buffer().resize(outSize);
            buffer().fill('\0');

            // Fill the buffer
            PFORT_CONF_RULES rules = PFORT_CONF_RULES(data());
            rules->max_rule_id = wra.maxRuleId;
            rules->glob.pre_rule_id = wra.globPreRuleId;
            rules->glob.post_rule_id = wra.globPostRuleId;
        }

        return writeRule(rule, wra);
    });
}

void ConfBuffer::writeRuleFlag(int ruleId, bool enabled)
{
    // Resize the buffer
    const int flagSize = sizeof(FORT_CONF_RULE_FLAG);

    buffer().resize(flagSize);

    // Fill the buffer
    char *data = buffer().data();

    PFORT_CONF_RULE_FLAG confRuleFlag = PFORT_CONF_RULE_FLAG(data);

    confRuleFlag->rule_id = ruleId;
    confRuleFlag->enabled = enabled;
}

bool ConfBuffer::validateRuleText(const QString &ruleText)
{
    int filtersCount;
    return writeRuleText(ruleText, filtersCount);
}

bool ConfBuffer::writeRule(const Rule &rule, const WalkRulesArgs &wra)
{
    const quint16 ruleId = rule.ruleId;
    const auto ruleSetInfo = wra.ruleSetMap[ruleId];

    // Write the rule's offset
    {
        int *ruleOffsets = (int *) (data() + FORT_CONF_RULES_DATA_OFF) - 1; // exclude zero index
        ruleOffsets[ruleId] = int(buffer().size() - FORT_CONF_RULES_DATA_OFF);
    }

    const char *setIndexes = (const char *) (wra.ruleSetIds.constData() + ruleSetInfo.index);
    const auto ruleSet = QByteArray::fromRawData(
            setIndexes, FORT_CONF_RULES_SET_INDEXES_SIZE(ruleSetInfo.count));

    return writeRuleData(rule, ruleSet);
}

bool ConfBuffer::writeRuleData(const Rule &rule, const QByteArray &ruleSet)
{
    FORT_CONF_RULE confRule = confRuleFlags(rule);

    const bool hasZones = confRule.has_zones;
    const bool hasFilters = confRule.has_filters;

    const int ruleSetCount = int(ruleSet.size() / sizeof(quint16));
    confRule.set_count = ruleSetCount;

    // Resize the buffer
    const int oldSize = buffer().size();

    buffer().resize(oldSize + FORT_CONF_RULE_SIZE(&confRule));

    // Fill the buffer
    char *data = this->data() + oldSize;

    // Write the rule
    {
        *(PFORT_CONF_RULE(data)) = confRule;

        data += sizeof(FORT_CONF_RULE);
    }

    // Write the rule's zones
    if (hasZones) {
        PFORT_CONF_RULE_ZONES ruleZones = PFORT_CONF_RULE_ZONES(data);
        *ruleZones = rule.zones;

        data += sizeof(FORT_CONF_RULE_ZONES);
    }

    // Write the rule's set
    if (ruleSetCount != 0) {
        ConfData(data).writeArray(ruleSet);
    }

    // Write the rule's text
    if (hasFilters) {
        int filtersCount = 0;
        if (!writeRuleText(rule.ruleText, filtersCount))
            return false;

        if (filtersCount == 0) {
            PFORT_CONF_RULE oldConfRule = PFORT_CONF_RULE(this->data() + oldSize);

            oldConfRule->has_filters = false;
        }
    }

    return true;
}

bool ConfBuffer::writeRuleText(const QString &ruleText, int &filtersCount)
{
    RuleTextParser parser(ruleText);

    if (!parser.parse()) {
        setErrorMessage(parser.errorMessage());
        return false;
    }

    filtersCount = parser.ruleFilters().size();
    if (filtersCount == 0)
        return true;

    const auto &ruleFilter = parser.ruleFilters().first();
    Q_ASSERT(ruleFilter.isTypeList() || filtersCount == 1);

    return writeRuleFilter(ruleFilter);
}

bool ConfBuffer::writeRuleFilter(const RuleFilter &ruleFilter)
{
    // Resize the buffer
    const int oldSize = buffer().size();

    buffer().resize(oldSize + sizeof(FORT_CONF_RULE_FILTER));

    // Fill the buffer
    const bool ok = ruleFilter.isTypeList() ? writeRuleFilterList(ruleFilter)
                                            : writeRuleFilterValues(ruleFilter);

    if (ok) {
        PFORT_CONF_RULE_FILTER confFilter = PFORT_CONF_RULE_FILTER(data() + oldSize);

        confFilter->is_not = ruleFilter.isNot;
        confFilter->equal_values = ruleFilter.equalValues;
        confFilter->is_empty = ruleFilter.isEmpty();
        confFilter->type = ruleFilter.type;

        const quint32 filterSize = buffer().size() - oldSize;
        Q_ASSERT(filterSize > 0);

        if (filterSize > FORT_CONF_RULE_FILTER_SIZE_MAX) {
            setErrorMessage(tr("Too many values"));
            return false;
        }

        confFilter->size = filterSize;
    }

    return ok;
}

bool ConfBuffer::writeRuleFilterList(const RuleFilter &ruleListFilter)
{
    const RuleFilter *ruleFilter = &ruleListFilter + 1;
    int count = ruleListFilter.filterListCount;

    for (; --count >= 0; ++ruleFilter) {
        if (!writeRuleFilter(*ruleFilter))
            return false;

        if (ruleFilter->isTypeList()) {
            const int filterListCount = ruleFilter->filterListCount;

            count -= filterListCount;
            ruleFilter += filterListCount;
        }
    }

    return true;
}

bool ConfBuffer::writeRuleFilterValues(const RuleFilter &ruleFilter)
{
    QScopedPointer<ValueRange> range(ValueRangeUtil::createRangeByType(ruleFilter.type));

    if (!range->fromList(ruleFilter.values)) {
        setErrorMessage(range->errorLineAndMessageDetails());
        return false;
    }

    if (!range->checkSize()) {
        setErrorMessage(tr("Too many values"));
        return false;
    }

    // Resize the buffer
    const int oldSize = buffer().size();
    const int newSize = oldSize + range->sizeToWrite();

    buffer().resize(newSize);

    // Fill the buffer
    ConfData confData(data() + oldSize);

    range->write(confData);

    return true;
}

void ConfBuffer::writeGroups(const ConfGroupsWalker &confGroupsWalker, quint32 activeMask)
{
    // Resize the buffer
    buffer().resize(sizeof(FORT_CONF_GROUPS));
    buffer().fill('\0');

    // Fill the buffer
    PFORT_CONF_GROUPS confGroups = PFORT_CONF_GROUPS(buffer().data());

    confGroupsWalker.walkGroups([&](Group &group) -> bool {
        if (Q_UNLIKELY(group.groupId <= 0 || group.groupId > ConfUtil::groupMaxCount()))
            return true; // skip an out of range Group

        const int groupIndex = group.groupId - 1;
        const quint32 groupBit = (quint32(1) << groupIndex);

        confGroups->mask |= groupBit;

        if (group.exclusive) {
            confGroups->exclusive_mask |= groupBit;
        }

        confGroups->rule_ids[groupIndex] = group.ruleId;

        return true;
    });

    confGroups->enabled_mask = (activeMask & confGroups->mask);
}

void ConfBuffer::writeGroupFlags(quint32 activeMask)
{
    // Resize the buffer
    buffer().resize(sizeof(FORT_CONF_GROUP_FLAGS));

    // Fill the buffer
    PFORT_CONF_GROUP_FLAGS confGroupFlags = PFORT_CONF_GROUP_FLAGS(buffer().data());

    confGroupFlags->enabled_mask = activeMask;
}

void ConfBuffer::writeSpeedLimits(
        const ConfSpeedLimitsWalker &confSpeedLimitsWalker, quint32 activeMask)
{
    // Resize the buffer
    buffer().resize(sizeof(FORT_CONF_SPEED_LIMITS));
    buffer().fill('\0');

    // Fill the buffer
    PFORT_CONF_SPEED_LIMITS confLimits = PFORT_CONF_SPEED_LIMITS(buffer().data());

    confSpeedLimitsWalker.walkSpeedLimits([&](const SpeedLimit &limit) -> bool {
        if (Q_UNLIKELY(limit.limitId <= 0 || limit.limitId > ConfUtil::speedLimitMaxCount()))
            return true; // skip an out of range Speed Limit

        if (limit.kbps == 0)
            return true; // the zero speed means no limit, it would stall the queue

        const int limitIndex = limit.limitId - 1;
        const quint32 limitBit = (quint32(1) << limitIndex);

        confLimits->mask |= limitBit;

        PFORT_SPEED_LIMIT confLimit = &confLimits->limits[limitIndex];

        confLimit->plr = limit.packetLoss;
        confLimit->latency_ms = limit.latency;
        confLimit->buffer_bytes = limit.bufferSize;
        confLimit->bps = quint64(limit.kbps) * (1024LL / 8); /* to bytes per second */

        return true;
    });

    confLimits->enabled_mask = (activeMask & confLimits->mask);
}

void ConfBuffer::writeSpeedLimitFlags(quint32 enabledMask)
{
    // Resize the buffer
    buffer().resize(sizeof(FORT_CONF_SPEED_LIMIT_FLAGS));

    // Fill the buffer
    PFORT_CONF_SPEED_LIMIT_FLAGS confLimitFlags = PFORT_CONF_SPEED_LIMIT_FLAGS(buffer().data());

    confLimitFlags->enabled_mask = enabledMask;
}

void ConfBuffer::writePeriods(quint64 activeMask)
{
    // Resize the buffer
    buffer().resize(sizeof(FORT_CONF_PERIODS));

    // Fill the buffer
    PFORT_CONF_PERIODS confPeriods = PFORT_CONF_PERIODS(buffer().data());

    confPeriods->active_mask = activeMask;
}
