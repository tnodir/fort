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
#include <util/net/valuerangeutil.h>
#include <util/stringutil.h>

#include "confappswalker.h"
#include "confrodata.h"
#include "confruleswalker.h"
#include "confutil.h"
#include "ruletextparser.h"

#define APP_GROUP_MAX      FORT_CONF_GROUP_MAX
#define APP_GROUP_NAME_MAX 128
#define APP_PATH_MAX       FORT_CONF_APP_PATH_MAX

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

bool ConfBuffer::writeConf(
        const FirewallConf &conf, const ConfAppsWalker *confAppsWalker, EnvManager &envManager)
{
    WriteConfArgs wca = {
        .conf = conf,
        .ad = { .addressRanges = addrranges_arr_t(conf.addressGroups().size()) },
    };

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
        app.logAllowedConn = appGroup->logConn();
        app.logBlockedConn = appGroup->logBlocked();
        app.groupId = i;

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
                .apply_parent = app.applyParent,
                .apply_child = app.applyChild,
                .apply_spec_child = app.applySpecChild,
                .kill_child = app.killChild,
                .lan_only = app.lanOnly,
                .log_allowed_conn = app.logAllowedConn,
                .log_blocked_conn = app.logBlockedConn,
                .blocked = app.blocked,
                .kill_process = app.killProcess,
                .is_new = isNew,
                .found = true,
        },
        .group_id = app.groupId,
        .in_limit_id = app.inLimitId,
        .out_limit_id = app.outLimitId,
        .rule_id = app.ruleId,
        .zones = app.zones,
    };

    appsMap.insert(kernelPath, appData);

    m_driveMask |= FileUtil::driveMaskByPath(app.appPath);

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

    FORT_CONF_RULE confRule;
    confRule.enabled = rule.enabled;
    confRule.blocked = rule.blocked;
    confRule.exclusive = rule.exclusive;
    confRule.terminate = rule.terminate;
    confRule.term_blocked = rule.terminateBlocked;

    const bool hasZones = (rule.zones.accept_mask != 0 || rule.zones.reject_mask != 0);
    confRule.has_zones = hasZones;

    const bool hasFilters = !rule.ruleText.isEmpty();
    confRule.has_filters = hasFilters;

    const int ruleSetCount = ruleSetInfo.count;
    confRule.set_count = ruleSetCount;

    // Resize the buffer
    const int oldSize = buffer().size();

    buffer().resize(oldSize + FORT_CONF_RULE_SIZE(&confRule));

    // Fill the buffer
    char *data = this->data();

    // Write the rule's offset
    {
        int *ruleOffsets = (int *) (data + FORT_CONF_RULES_DATA_OFF) - 1; // exclude zero index
        ruleOffsets[ruleId] = oldSize - FORT_CONF_RULES_DATA_OFF;

        data += oldSize;
    }

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
        const char *setIndexes = (const char *) &wra.ruleSetIds[ruleSetInfo.index];
        const auto array =
                QByteArray::fromRawData(setIndexes, FORT_CONF_RULES_SET_INDEXES_SIZE(ruleSetCount));

        ConfData(data).writeArray(array);
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
    Q_ASSERT(ruleFilter.isTypeList());

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
        confFilter->is_empty = !ruleFilter.hasValues();
        confFilter->type = ruleFilter.type;

        const quint32 filterSize = buffer().size() - oldSize;
        Q_ASSERT(filterSize > 0);

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
