#include "confutil.h"

#include <QRegularExpression>

#include <common/fortconf.h>
#include <fort_version.h>

#include <conf/addressgroup.h>
#include <conf/appgroup.h>
#include <conf/firewallconf.h>
#include <driver/drivercommon.h>
#include <manager/envmanager.h>
#include <util/dateutil.h>
#include <util/fileutil.h>
#include <util/stringutil.h>

#include "confappswalker.h"

#define APP_GROUP_MAX      FORT_CONF_GROUP_MAX
#define APP_GROUP_NAME_MAX 128
#define APP_PATH_MAX       (FORT_CONF_APP_PATH_MAX / sizeof(wchar_t))

namespace {

void writeConfFlags(const FirewallConf &conf, PFORT_CONF_FLAGS confFlags)
{
    confFlags->prov_boot = conf.provBoot();
    confFlags->filter_enabled = conf.filterEnabled();
    confFlags->filter_locals = conf.filterLocals();
    confFlags->stop_traffic = conf.stopTraffic();
    confFlags->stop_inet_traffic = conf.stopInetTraffic();
    confFlags->allow_all_new = conf.allowAllNew();

    confFlags->app_block_all = conf.appBlockAll();
    confFlags->app_allow_all = conf.appAllowAll();

    confFlags->log_blocked = conf.logBlocked();
    confFlags->log_stat = conf.logStat();
    confFlags->log_stat_no_filter = conf.logStatNoFilter();

    confFlags->log_allowed_ip = conf.logAllowedIp();
    confFlags->log_blocked_ip = conf.logBlockedIp();

    confFlags->group_bits = conf.appGroupBits();
}

}

ConfUtil::ConfUtil(QObject *parent) : QObject(parent) { }

int ConfUtil::zoneMaxCount()
{
    return FORT_CONF_ZONE_MAX;
}

void ConfUtil::setErrorMessage(const QString &errorMessage)
{
    if (m_errorMessage != errorMessage) {
        m_errorMessage = errorMessage;
        emit errorMessageChanged();
    }
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

    appentry_map_t wildAppsMap;
    appentry_map_t prefixAppsMap;
    appentry_map_t exeAppsMap;

    quint32 wildAppsSize = 0;
    quint32 prefixAppsSize = 0;
    quint32 exeAppsSize = 0;

    if (!parseAppGroups(envManager, conf.appGroups(), appPeriods, appPeriodsCount, wildAppsMap,
                prefixAppsMap, exeAppsMap, wildAppsSize, prefixAppsSize, exeAppsSize)
            || !parseExeApps(confAppsWalker, exeAppsMap, exeAppsSize))
        return 0;

    const quint32 appsSize = wildAppsSize + prefixAppsSize + exeAppsSize;
    if (appsSize > FORT_CONF_APPS_LEN_MAX) {
        setErrorMessage(tr("Too many application paths"));
        return 0;
    }

    // Fill the buffer
    const size_t confIoSize = FORT_CONF_IO_CONF_OFF + FORT_CONF_DATA_OFF + addressGroupsSize
            + FORT_CONF_STR_DATA_SIZE(conf.appGroups().size() * sizeof(FORT_PERIOD)) // appPeriods
            + FORT_CONF_STR_DATA_SIZE(wildAppsSize)
            + FORT_CONF_STR_HEADER_SIZE(prefixAppsMap.size())
            + FORT_CONF_STR_DATA_SIZE(prefixAppsSize) + FORT_CONF_STR_DATA_SIZE(exeAppsSize);

    buf.reserve(confIoSize);

    writeConf(buf.data(), conf, addressRanges, addressGroupOffsets, appPeriods, appPeriodsCount,
            wildAppsMap, prefixAppsMap, exeAppsMap);

    return int(confIoSize);
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

int ConfUtil::writeAppEntry(int groupIndex, bool useGroupPerm, bool applyChild, bool blocked,
        bool alerted, bool isNew, const QString &appPath, QByteArray &buf)
{
    appentry_map_t exeAppsMap;
    quint32 exeAppsSize = 0;

    if (!addApp(groupIndex, useGroupPerm, applyChild, blocked, alerted, isNew, appPath, exeAppsMap,
                exeAppsSize))
        return 0;

    buf.reserve(exeAppsSize);

    // Fill the buffer
    char *data = (char *) buf.data();

    writeApps(&data, exeAppsMap);

    return int(exeAppsSize);
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

        const int zoneIndex = DriverCommon::bitScanForward(zonesMask);
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

        const int incIp4Size = addressRange.includeRange().ip4Size();
        const int incPair4Size = addressRange.includeRange().pair4Size();

        const int incIp6Size = addressRange.includeRange().ip6Size();
        const int incPair6Size = addressRange.includeRange().pair6Size();

        const int excIp4Size = addressRange.excludeRange().ip4Size();
        const int excPair4Size = addressRange.excludeRange().pair4Size();

        const int excIp6Size = addressRange.excludeRange().ip6Size();
        const int excPair6Size = addressRange.excludeRange().pair6Size();

        if ((incIp4Size + incPair4Size) > FORT_CONF_IP_MAX
                || (incIp6Size + incPair6Size) > FORT_CONF_IP_MAX
                || (excIp4Size + excPair4Size) > FORT_CONF_IP_MAX
                || (excIp6Size + excPair6Size) > FORT_CONF_IP_MAX) {
            setErrorMessage(tr("Too many IP addresses"));
            return false;
        }

        addressGroupOffsets.append(addressGroupsSize);

        addressGroupsSize += FORT_CONF_ADDR_GROUP_OFF
                + FORT_CONF_ADDR_LIST_SIZE(incIp4Size, incPair4Size, incIp6Size, incPair6Size)
                + FORT_CONF_ADDR_LIST_SIZE(excIp4Size, excPair4Size, excIp6Size, excPair6Size);
    }

    return true;
}

bool ConfUtil::parseAppGroups(EnvManager &envManager, const QList<AppGroup *> &appGroups,
        chars_arr_t &appPeriods, quint8 &appPeriodsCount, appentry_map_t &wildAppsMap,
        appentry_map_t &prefixAppsMap, appentry_map_t &exeAppsMap, quint32 &wildAppsSize,
        quint32 &prefixAppsSize, quint32 &exeAppsSize)
{
    const int groupsCount = appGroups.size();
    if (groupsCount < 1 || groupsCount > APP_GROUP_MAX) {
        setErrorMessage(
                tr("Number of Application Groups must be between 1 and %1").arg(APP_GROUP_MAX));
        return false;
    }

    envManager.clearCache(); // evaluate env vars on each save to GC

    for (int i = 0; i < groupsCount; ++i) {
        const AppGroup *appGroup = appGroups.at(i);

        const QString name = appGroup->name();
        if (name.size() > APP_GROUP_NAME_MAX) {
            setErrorMessage(
                    tr("Length of Application Group's Name must be < %1").arg(APP_GROUP_NAME_MAX));
            return false;
        }

        const bool applyChild = appGroup->applyChild();
        const auto blockText = envManager.expandString(appGroup->blockText());
        const auto allowText = envManager.expandString(appGroup->allowText());

        if (!parseAppsText(i, applyChild, /*blocked=*/true, blockText, wildAppsMap, prefixAppsMap,
                    exeAppsMap, wildAppsSize, prefixAppsSize, exeAppsSize)
                || !parseAppsText(i, applyChild, /*blocked=*/false, allowText, wildAppsMap,
                        prefixAppsMap, exeAppsMap, wildAppsSize, prefixAppsSize, exeAppsSize))
            return false;

        // Enabled Period
        parseAppPeriod(appGroup, appPeriods, appPeriodsCount);
    }

    return true;
}

bool ConfUtil::parseExeApps(
        ConfAppsWalker *confAppsWalker, appentry_map_t &exeAppsMap, quint32 &exeAppsSize)
{
    if (Q_UNLIKELY(!confAppsWalker))
        return true;

    return confAppsWalker->walkApps(
            [&](int groupIndex, bool useGroupPerm, bool applyChild, bool blocked, bool alerted,
                    const QString &appPath) -> bool {
                return addApp(groupIndex, useGroupPerm, applyChild, blocked, alerted,
                        /*isNew=*/true, appPath, exeAppsMap, exeAppsSize);
            });
}

bool ConfUtil::parseAppsText(int groupIndex, bool applyChild, bool blocked, const QString &text,
        appentry_map_t &wildAppsMap, appentry_map_t &prefixAppsMap, appentry_map_t &exeAppsMap,
        quint32 &wildAppsSize, quint32 &prefixAppsSize, quint32 &exeAppsSize)
{
    const auto lines = StringUtil::tokenizeView(text, QLatin1Char('\n'));

    for (const auto &line : lines) {
        const auto lineTrimmed = line.trimmed();
        if (lineTrimmed.isEmpty() || lineTrimmed.startsWith('#')) // commented line
            continue;

        bool isWild = false;
        bool isPrefix = false;
        const QString appPath = parseAppPath(lineTrimmed, isWild, isPrefix);
        if (appPath.isEmpty())
            continue;

        if (!addParsedApp(groupIndex, applyChild, blocked, isWild, isPrefix, appPath, wildAppsMap,
                    prefixAppsMap, exeAppsMap, wildAppsSize, prefixAppsSize, exeAppsSize))
            return false;
    }

    return true;
}

bool ConfUtil::addParsedApp(int groupIndex, bool applyChild, bool blocked, bool isWild,
        bool isPrefix, const QString &appPath, appentry_map_t &wildAppsMap,
        appentry_map_t &prefixAppsMap, appentry_map_t &exeAppsMap, quint32 &wildAppsSize,
        quint32 &prefixAppsSize, quint32 &exeAppsSize)
{
    appentry_map_t &appsMap = isWild ? wildAppsMap : (isPrefix ? prefixAppsMap : exeAppsMap);
    quint32 &appsSize = isWild ? wildAppsSize : (isPrefix ? prefixAppsSize : exeAppsSize);

    return addApp(groupIndex, /*useGroupPerm=*/true, applyChild, blocked,
            /*alerted=*/false, /*isNew=*/true, appPath, appsMap, appsSize, /*canOverwrite=*/false);
}

bool ConfUtil::addApp(int groupIndex, bool useGroupPerm, bool applyChild, bool blocked,
        bool alerted, bool isNew, const QString &appPath, appentry_map_t &appsMap,
        quint32 &appsSize, bool canOverwrite)
{
    const QString kernelPath = FileUtil::pathToKernelPath(appPath);

    if (appsMap.contains(kernelPath)) {
        if (!canOverwrite)
            return true;
    }

    if (kernelPath.size() > int(APP_PATH_MAX)) {
        setErrorMessage(tr("Length of Application's Path must be < %1").arg(APP_PATH_MAX));
        return false;
    }

    const quint16 appPathLen = quint16(kernelPath.size()) * sizeof(wchar_t);
    const quint32 appSize = FORT_CONF_APP_ENTRY_SIZE(appPathLen);

    appsSize += appSize;

    FORT_APP_ENTRY appEntry;
    appEntry.v = 0;
    appEntry.path_len = appPathLen;
    appEntry.flags.group_index = quint8(groupIndex);
    appEntry.flags.use_group_perm = useGroupPerm;
    appEntry.flags.apply_child = applyChild;
    appEntry.flags.blocked = blocked;
    appEntry.flags.alerted = alerted;
    appEntry.flags.is_new = isNew;
    appEntry.flags.found = 1;

    appsMap.insert(kernelPath, appEntry.v);

    return true;
}

QString ConfUtil::parseAppPath(const StringView line, bool &isWild, bool &isPrefix)
{
    static const QRegularExpression wildMatcher("([*?[])");

    auto path = line;
    if (path.startsWith('"') && path.endsWith('"')) {
        path = path.mid(1, path.size() - 2);
    }

    if (path.isEmpty())
        return QString();

    const auto wildMatch = wildMatcher.match(path);
    if (wildMatch.hasMatch()) {
        if (wildMatch.capturedStart() == path.size() - 2 && wildMatch.capturedView().at(0) == '*'
                && path.endsWith('*')) {
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

        if (fromHour != 0 || fromMinute != 0 || toHour != 0 || toMinute != 0) {
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
        const chars_arr_t &appPeriods, quint8 appPeriodsCount, const appentry_map_t &wildAppsMap,
        const appentry_map_t &prefixAppsMap, const appentry_map_t &exeAppsMap)
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
    writeApps(&data, wildAppsMap);

    prefixAppsOff = CONF_DATA_OFFSET;
    writeApps(&data, prefixAppsMap, true);

    exeAppsOff = CONF_DATA_OFFSET;
    writeApps(&data, exeAppsMap);
#undef CONF_DATA_OFFSET

    writeAppGroupFlags(&drvConfIo->conf_group.log_conn, &drvConfIo->conf_group.fragment_bits, conf);

    writeLimits(drvConfIo->conf_group.limits, &drvConfIo->conf_group.limit_bits,
            &drvConfIo->conf_group.limit_2bits, conf.appGroups());

    writeConfFlags(conf, &drvConf->flags);

    DriverCommon::confAppPermsMaskInit(drvConf);

    drvConf->app_periods_n = appPeriodsCount;

    drvConf->wild_apps_n = quint16(wildAppsMap.size());
    drvConf->prefix_apps_n = quint16(prefixAppsMap.size());
    drvConf->exe_apps_n = quint16(exeAppsMap.size());

    drvConf->addr_groups_off = addrGroupsOff;

    drvConf->app_periods_off = appPeriodsOff;

    drvConf->wild_apps_off = wildAppsOff;
    drvConf->prefix_apps_off = prefixAppsOff;
    drvConf->exe_apps_off = exeAppsOff;
}

void ConfUtil::writeAppGroupFlags(
        quint16 *logConnBits, quint16 *fragmentBits, const FirewallConf &conf)
{
    *logConnBits = 0;
    *fragmentBits = 0;
    int i = 0;
    for (const AppGroup *appGroup : conf.appGroups()) {
        if (appGroup->logConn()) {
            *logConnBits |= (1 << i);
        }
        if (appGroup->fragmentPacket()) {
            *fragmentBits |= (1 << i);
        }
        ++i;
    }
}

void ConfUtil::writeLimits(struct fort_traf *limits, quint16 *limitBits, quint32 *limit2Bits,
        const QList<AppGroup *> &appGroups)
{
    PFORT_TRAF limit = &limits[0];

    *limitBits = 0;
    *limit2Bits = 0;

    const int groupsCount = appGroups.size();
    for (int i = 0; i < groupsCount; ++i, ++limit) {
        const AppGroup *appGroup = appGroups.at(i);

        limit->in_bytes = appGroup->enabledSpeedLimitIn() * 1024 / 2;
        limit->out_bytes = appGroup->enabledSpeedLimitOut() * 1024 / 2;

        const bool isLimitIn = (limit->in_bytes != 0);
        const bool isLimitOut = (limit->out_bytes != 0);

        if (isLimitIn || isLimitOut) {
            *limitBits |= (1 << i);

            if (isLimitIn) {
                *limit2Bits |= (1 << (i * 2));
            }
            if (isLimitOut) {
                *limit2Bits |= (1 << (i * 2 + 1));
            }
        }
    }
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

    bufSize -= FORT_CONF_ADDR4_LIST_SIZE(addr_list->ip_n, addr_list->pair_n);
    if (bufSize < 0)
        return false;

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

    bufSize -= FORT_CONF_ADDR6_LIST_SIZE(addr_list->ip_n, addr_list->pair_n);
    if (bufSize < 0)
        return false;

    ipRange.ip6Array().resize(addr_list->ip_n);
    ipRange.pair6FromArray().resize(addr_list->pair_n);
    ipRange.pair6ToArray().resize(addr_list->pair_n);

    loadIp6Array(data, ipRange.ip6Array());
    loadIp6Array(data, ipRange.pair6FromArray());
    loadIp6Array(data, ipRange.pair6ToArray());

    return true;
}

void ConfUtil::writeApps(char **data, const appentry_map_t &apps, bool useHeader)
{
    quint32 *offp = (quint32 *) *data;
    const quint32 offTableSize = quint32(useHeader ? FORT_CONF_STR_HEADER_SIZE(apps.size()) : 0);
    char *p = *data + offTableSize;
    quint32 off = 0;

    if (useHeader) {
        *offp++ = 0;
    }

    auto it = apps.constBegin();
    const auto end = apps.constEnd();
    for (; it != end; ++it) {
        const QString &appPath = it.key();

        FORT_APP_ENTRY appEntry;
        appEntry.v = it.value();

        PFORT_APP_ENTRY entry = (PFORT_APP_ENTRY) p;
        *entry++ = appEntry;

        wchar_t *pathArray = (wchar_t *) entry;
        appPath.toWCharArray(pathArray);
        pathArray[appEntry.path_len / sizeof(wchar_t)] = '\0';

        const quint32 appSize = FORT_CONF_APP_ENTRY_SIZE(appEntry.path_len);

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
