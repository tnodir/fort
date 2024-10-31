#include "confbuffer.h"

#include <QHash>
#include <QMap>

#include <fort_version.h>

#include <conf/addressgroup.h>
#include <conf/app.h>
#include <conf/appgroup.h>
#include <conf/firewallconf.h>
#include <conf/rule.h>
#include <manager/envmanager.h>
#include <util/bitutil.h>
#include <util/fileutil.h>
#include <util/stringutil.h>

#include "confappswalker.h"
#include "confconstdata.h"
#include "confruleswalker.h"
#include "confutil.h"
#include "ruletextparser.h"

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

void ConfBuffer::writeServices(const QVector<ServiceInfo> &services, int runningServicesCount)
{
    // Resize the buffer to max size
    const int servicesSize =
            FORT_SERVICE_INFO_LIST_MIN_SIZE + runningServicesCount * FORT_SERVICE_INFO_MAX_SIZE;

    buffer().resize(servicesSize);

    // Fill the buffer
    char *data = buffer().data();

    int outSize = writeServicesHeader(data, runningServicesCount);

    for (const ServiceInfo &info : services) {
        if (!info.isRunning)
            continue;

        outSize += writeServiceInfo(data + outSize, info);
    }

    buffer().resize(outSize); // shrink to actual size
}

bool ConfBuffer::write(
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

    if (!parseAppGroups(envManager, conf.appGroups(), opt))
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

bool ConfBuffer::writeRules(const ConfRulesWalker &confRulesWalker)
{
    ruleset_map_t ruleSetMap;
    ruleid_arr_t ruleSetIds;
    int maxRuleId;

    return confRulesWalker.walkRules(ruleSetMap, ruleSetIds, maxRuleId, [&](Rule &rule) -> bool {
        if (buffer().isEmpty()) {
            const int outSize = FORT_CONF_RULES_DATA_OFF + FORT_CONF_RULES_OFFSETS_SIZE(maxRuleId);

            buffer().resize(outSize);
            buffer().fill('\0');

            PFORT_CONF_RULES rules = (PFORT_CONF_RULES) buffer().data();
            rules->max_rule_id = maxRuleId;
        }

        writeRule(rule, ruleSetMap, ruleSetIds);

        return true;
    });
}

void ConfBuffer::writeZone(const IpRange &ipRange)
{
    // Resize the buffer
    const int addrSize = FORT_CONF_ADDR_LIST_SIZE(
            ipRange.ip4Size(), ipRange.pair4Size(), ipRange.ip6Size(), ipRange.pair6Size());

    buffer().resize(addrSize);

    // Fill the buffer
    char *data = buffer().data();

    ConfData(data).writeAddressList(ipRange);
}

void ConfBuffer::writeZones(quint32 zonesMask, quint32 enabledMask, quint32 dataSize,
        const QList<QByteArray> &zonesData)
{
    // Resize the buffer
    const int zonesSize = FORT_CONF_ZONES_DATA_OFF + dataSize;

    buffer().resize(zonesSize);

    // Fill the buffer
    char *data = buffer().data();

    PFORT_CONF_ZONES confZones = PFORT_CONF_ZONES(data);

    memset(confZones, 0, sizeof(FORT_CONF_ZONES_DATA_OFF));

    confZones->mask = zonesMask;
    confZones->enabled_mask = enabledMask;

    data = confZones->data;

    ConfData confUtilData(data);

    for (const auto &zoneData : zonesData) {
        Q_ASSERT(!zoneData.isEmpty());

        const int zoneIndex = BitUtil::bitScanForward(zonesMask);
        const quint32 zoneMask = (quint32(1) << zoneIndex);

#define CONF_DATA_OFFSET quint32(data - confZones->data)
        confZones->addr_off[zoneIndex] = CONF_DATA_OFFSET;

        confUtilData.writeArray(zoneData);
        confUtilData.migrateZoneData(zoneData);
#undef CONF_DATA_OFFSET

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

    return ConfConstData(data).loadAddressList(ipRange, bufSize);
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

bool ConfBuffer::parseAppGroups(
        EnvManager &envManager, const QList<AppGroup *> &appGroups, AppParseOptions &opt)
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

bool ConfBuffer::parseAppLine(App &app, const QStringView &line, AppParseOptions &opt)
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
                .apply_parent = app.applyParent,
                .apply_child = app.applyChild,
                .apply_spec_child = app.applySpecChild,
                .kill_child = app.killChild,
                .lan_only = app.lanOnly,
                .log_blocked = app.logBlocked,
                .log_conn = app.logConn,
                .blocked = app.blocked,
                .kill_process = app.killProcess,
        },
        .is_new = isNew,
        .found = true,
        .rule_id = app.ruleId,
        .accept_zones = quint16(app.acceptZones),
        .reject_zones = quint16(app.rejectZones),
    };

    appsMap.insert(kernelPath, appData);

    m_driveMask |= FileUtil::driveMaskByPath(app.appPath);

    return true;
}

void ConfBuffer::writeRule(
        const Rule &rule, const ruleset_map_t &ruleSetMap, const ruleid_arr_t &ruleSetIds)
{
    const int ruleId = rule.ruleId;
    const auto ruleSetInfo = ruleSetMap[ruleId];

    FORT_CONF_RULE confRule;
    confRule.enabled = rule.enabled;
    confRule.blocked = rule.blocked;
    confRule.exclusive = rule.exclusive;

    const bool hasZones = (rule.acceptZones != 0 || rule.rejectZones != 0);
    confRule.has_zones = hasZones;

    const bool hasExpr = !rule.ruleText.isEmpty();
    confRule.has_expr = hasExpr;

    const int ruleSetCount = ruleSetInfo.count;
    confRule.set_count = ruleSetCount;

    // Resize the buffer
    const int oldSize = buffer().size();

    buffer().resize(oldSize + FORT_CONF_RULE_SIZE(&confRule));

    // Fill the buffer
    char *data = buffer().data();

    // Write the rule's offset
    {
        int *ruleOffsets = (int *) (data + FORT_CONF_RULES_DATA_OFF);
        ruleOffsets[ruleId] = oldSize;

        data += oldSize;
    }

    // Write the rule
    {
        *((PFORT_CONF_RULE) data) = confRule;

        data += sizeof(FORT_CONF_RULE);
    }

    // Write the rule's zones
    if (hasZones) {
        PFORT_CONF_RULE_ZONES ruleZones = (PFORT_CONF_RULE_ZONES) data;
        ruleZones->accept_zones = rule.acceptZones;
        ruleZones->reject_zones = rule.rejectZones;

        data += sizeof(FORT_CONF_RULE_ZONES);
    }

    // Write the rule's set
    if (ruleSetCount != 0) {
        const auto array = QByteArray::fromRawData(
                (const char *) &ruleSetIds[ruleSetInfo.index], ruleSetCount * sizeof(quint16));

        ConfData(data).writeArray(array);
    }

    // Write the rule's conditions
    if (hasExpr) {
        writeRuleText(rule.ruleText);
    }
}

void ConfBuffer::writeRuleText(const QString &ruleText)
{
    RuleTextParser parser(ruleText);

    while (parser.parse()) {
        // TODO
    }
}
