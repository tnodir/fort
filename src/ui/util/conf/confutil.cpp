#include "confutil.h"

#include <QRegularExpression>

#define UCHAR   quint8
#define UINT16  quint16
#define UINT32  quint32
#define UINT64  quint64

#include "../../common/fortconf.h"
#include "../../common/version.h"
#include "../../conf/addressgroup.h"
#include "../../conf/appgroup.h"
#include "../../conf/firewallconf.h"
#include "../../fortcommon.h"
#include "../../util/conf/confappswalker.h"
#include "../dateutil.h"
#include "../envmanager.h"
#include "../fileutil.h"
#include "../net/ip4range.h"

#define APP_GROUP_MAX       FORT_CONF_GROUP_MAX
#define APP_GROUP_NAME_MAX  128
#define APP_PATH_MAX        (FORT_CONF_APP_PATH_MAX / sizeof(wchar_t))

namespace {

void bufferReserve(QByteArray &buf, int size)
{
    buf.reserve(size + 1);  // + internal terminating zero
}

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

    confFlags->group_bits = conf.appGroupBits();
}

}

ConfUtil::ConfUtil(QObject *parent) :
    QObject(parent)
{
}

void ConfUtil::setErrorMessage(const QString &errorMessage)
{
    if (m_errorMessage != errorMessage) {
        m_errorMessage = errorMessage;
        emit errorMessageChanged();
    }
}

int ConfUtil::write(const FirewallConf &conf,
                    ConfAppsWalker *confAppsWalker,
                    EnvManager &envManager, QByteArray &buf)
{
    quint32 addressGroupsSize = 0;
    longs_arr_t addressGroupOffsets;
    addrranges_arr_t addressRanges(conf.addressGroups().size());

    if (!parseAddressGroups(conf.addressGroups(), addressRanges,
                            addressGroupOffsets, addressGroupsSize))
        return 0;

    quint8 appPeriodsCount = 0;
    chars_arr_t appPeriods;

    appentry_map_t wildAppsMap;
    appentry_map_t prefixAppsMap;
    appentry_map_t exeAppsMap;

    quint32 wildAppsSize = 0;
    quint32 prefixAppsSize = 0;
    quint32 exeAppsSize = 0;

    if (!parseAppGroups(envManager, conf.appGroups(),
                        appPeriods, appPeriodsCount,
                        wildAppsMap, prefixAppsMap, exeAppsMap,
                        wildAppsSize, prefixAppsSize, exeAppsSize)
            || !parseExeApps(confAppsWalker, exeAppsMap, exeAppsSize))
        return 0;

    const quint32 appsSize = wildAppsSize + prefixAppsSize + exeAppsSize;
    if (appsSize > FORT_CONF_APPS_LEN_MAX) {
        setErrorMessage(tr("Too many application paths"));
        return 0;
    }

    // Fill the buffer
    const int confIoSize = FORT_CONF_IO_CONF_OFF + FORT_CONF_DATA_OFF
            + addressGroupsSize
            + FORT_CONF_STR_DATA_SIZE(conf.appGroups().size()
                                      * sizeof(FORT_PERIOD))  // appPeriods
            + FORT_CONF_STR_DATA_SIZE(wildAppsSize)
            + FORT_CONF_STR_HEADER_SIZE(prefixAppsMap.size())
            + FORT_CONF_STR_DATA_SIZE(prefixAppsSize)
            + FORT_CONF_STR_DATA_SIZE(exeAppsSize);

    bufferReserve(buf, confIoSize);

    writeData(buf.data(), conf,
              addressRanges, addressGroupOffsets,
              appPeriods, appPeriodsCount,
              wildAppsMap, prefixAppsMap, exeAppsMap);

    return confIoSize;
}

int ConfUtil::writeFlags(const FirewallConf &conf, QByteArray &buf)
{
    const int flagsSize = sizeof(FORT_CONF_FLAGS);

    bufferReserve(buf, flagsSize);

    // Fill the buffer
    PFORT_CONF_FLAGS confFlags = (PFORT_CONF_FLAGS) buf.data();

    writeConfFlags(conf, confFlags);

    return flagsSize;
}

int ConfUtil::writeAppEntry(int groupIndex, bool useGroupPerm,
                            bool blocked, bool alerted, bool isNew,
                            const QString &appPath, QByteArray &buf)
{
    appentry_map_t exeAppsMap;
    quint32 exeAppsSize = 0;

    if (!addApp(groupIndex, useGroupPerm,
                blocked, alerted, isNew, appPath,
                exeAppsMap, exeAppsSize))
        return 0;

    bufferReserve(buf, exeAppsSize);

    // Fill the buffer
    char *data = (char *) buf.data();

    writeApps(&data, exeAppsMap);

    return int(exeAppsSize);
}

int ConfUtil::writeVersion(QByteArray &buf)
{
    const int verSize = sizeof(FORT_CONF_VERSION);

    bufferReserve(buf, verSize);

    // Fill the buffer
    PFORT_CONF_VERSION confVer = (PFORT_CONF_VERSION) buf.data();

    confVer->driver_version = DRIVER_VERSION;

    return verSize;
}

int ConfUtil::writeZone(const Ip4Range &ip4Range, QByteArray &buf)
{
    const int addrSize = FORT_CONF_ADDR_LIST_SIZE(
                ip4Range.ipSize(), ip4Range.pairSize());

    bufferReserve(buf, addrSize);

    // Fill the buffer
    char *data = (char *) buf.data();

    writeAddressList(&data, ip4Range);

    return addrSize;
}

bool ConfUtil::parseAddressGroups(const QList<AddressGroup *> &addressGroups,
                                  addrranges_arr_t &addressRanges,
                                  longs_arr_t &addressGroupOffsets,
                                  quint32 &addressGroupsSize)
{
    const int groupsCount = addressGroups.size();

    addressGroupsSize = quint32(groupsCount) * sizeof(quint32);  // offsets

    for (int i = 0; i < groupsCount; ++i) {
        AddressGroup *addressGroup = addressGroups.at(i);

        AddressRange &addressRange = addressRanges[i];
        addressRange.setIncludeAll(addressGroup->includeAll());
        addressRange.setExcludeAll(addressGroup->excludeAll());

        if (!addressRange.includeRange()
                .fromText(addressGroup->includeText())) {
            setErrorMessage(tr("Bad Include IP address: %1")
                            .arg(addressRange.includeRange()
                                 .errorLineAndMessage()));
            return false;
        }

        if (!addressRange.excludeRange()
                .fromText(addressGroup->excludeText())) {
            setErrorMessage(tr("Bad Exclude IP address: %1")
                            .arg(addressRange.excludeRange()
                                 .errorLineAndMessage()));
            return false;
        }

        const int incIpSize = addressRange.includeRange().ipSize();
        const int incPairSize = addressRange.includeRange().pairSize();

        const int excIpSize = addressRange.excludeRange().ipSize();
        const int excPairSize = addressRange.excludeRange().pairSize();

        if ((incIpSize + incPairSize) > FORT_CONF_IP_MAX
                || (excIpSize + excPairSize) > FORT_CONF_IP_MAX) {
            setErrorMessage(tr("Too many IP addresses"));
            return false;
        }

        addressGroupOffsets.append(addressGroupsSize);

        addressGroupsSize += FORT_CONF_ADDR_GROUP_OFF
                + FORT_CONF_ADDR_LIST_SIZE(incIpSize, incPairSize)
                + FORT_CONF_ADDR_LIST_SIZE(excIpSize, excPairSize);
    }

    return true;
}

bool ConfUtil::parseAppGroups(EnvManager &envManager,
                              const QList<AppGroup *> &appGroups,
                              chars_arr_t &appPeriods,
                              quint8 &appPeriodsCount,
                              appentry_map_t &wildAppsMap,
                              appentry_map_t &prefixAppsMap,
                              appentry_map_t &exeAppsMap,
                              quint32 &wildAppsSize,
                              quint32 &prefixAppsSize,
                              quint32 &exeAppsSize)
{
    const int groupsCount = appGroups.size();
    if (groupsCount < 1 || groupsCount > APP_GROUP_MAX) {
        setErrorMessage(tr("Number of Application Groups must be between 1 and %1")
                        .arg(APP_GROUP_MAX));
        return false;
    }

    envManager.clearCache();  // evaluate env vars on each save to GC

    for (int i = 0; i < groupsCount; ++i) {
        const AppGroup *appGroup = appGroups.at(i);

        const QString name = appGroup->name();
        if (name.size() > APP_GROUP_NAME_MAX) {
            setErrorMessage(tr("Length of Application Group's Name must be < %1")
                            .arg(APP_GROUP_NAME_MAX));
            return false;
        }

        const auto blockText = envManager.expandString(appGroup->blockText());
        const auto allowText = envManager.expandString(appGroup->allowText());

        if (!parseAppsText(i, true, blockText,
                       wildAppsMap, prefixAppsMap, exeAppsMap,
                       wildAppsSize, prefixAppsSize, exeAppsSize)
                || !parseAppsText(i, false, allowText,
                              wildAppsMap, prefixAppsMap, exeAppsMap,
                              wildAppsSize, prefixAppsSize, exeAppsSize))
            return false;

        // Enabled Period
        {
            quint8 fromHour = 0, fromMinute = 0;
            quint8 toHour = 0, toMinute = 0;

            if (appGroup->periodEnabled()) {
                DateUtil::parseTime(appGroup->periodFrom(), fromHour, fromMinute);
                DateUtil::parseTime(appGroup->periodTo(), toHour, toMinute);

                if (fromHour != 0 || fromMinute != 0
                        || toHour != 0 || toMinute != 0) {
                    ++appPeriodsCount;
                }
            }
            appPeriods.append(qint8(fromHour));
            appPeriods.append(qint8(fromMinute));
            appPeriods.append(qint8(toHour));
            appPeriods.append(qint8(toMinute));
        }
    }

    return true;
}

bool ConfUtil::parseExeApps(ConfAppsWalker *confAppsWalker,
                            appentry_map_t &exeAppsMap,
                            quint32 &exeAppsSize)
{
    if (Q_UNLIKELY(confAppsWalker == nullptr))
        return true;

    return confAppsWalker->walkApps([&](int groupIndex, bool useGroupPerm,
                                    bool blocked, bool alerted,
                                    const QString &appPath) -> bool {
        return addApp(groupIndex, useGroupPerm,
                      blocked, alerted, true, appPath,
                      exeAppsMap, exeAppsSize, false);
    });
}

bool ConfUtil::parseAppsText(int groupIndex, bool blocked, const QString &text,
                             appentry_map_t &wildAppsMap,
                             appentry_map_t &prefixAppsMap,
                             appentry_map_t &exeAppsMap,
                             quint32 &wildAppsSize,
                             quint32 &prefixAppsSize,
                             quint32 &exeAppsSize)
{
    for (const QStringRef &line :
             text.splitRef(QLatin1Char('\n'))) {
        const QStringRef lineTrimmed = line.trimmed();
        if (lineTrimmed.isEmpty()
                || lineTrimmed.startsWith('#'))  // commented line
            continue;

        bool isWild = false;
        bool isPrefix = false;
        const QString appPath = parseAppPath(lineTrimmed, isWild, isPrefix);
        if (appPath.isEmpty())
            continue;

        appentry_map_t &appsMap = isWild ? wildAppsMap
                                         : isPrefix ? prefixAppsMap
                                                    : exeAppsMap;
        quint32 &appsSize = isWild ? wildAppsSize
                                   : isPrefix ? prefixAppsSize
                                              : exeAppsSize;

        if (!addApp(groupIndex, true, blocked, false, true,
                    appPath, appsMap, appsSize))
            return false;
    }

    return true;
}

bool ConfUtil::addApp(int groupIndex, bool useGroupPerm,
                      bool blocked, bool alerted, bool isNew,
                      const QString &appPath, appentry_map_t &appsMap,
                      quint32 &appsSize, bool canCollide)
{
    const QString kernelPath = FileUtil::pathToKernelPath(appPath);

    if (appsMap.contains(kernelPath)) {
        if (!canCollide) {
            setErrorMessage(tr("Application '%1' already exists")
                            .arg(appPath));
            return false;
        }
        return true;
    }

    if (kernelPath.size() > int(APP_PATH_MAX)) {
        setErrorMessage(tr("Length of Application's Path must be < %1")
                        .arg(APP_PATH_MAX));
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
    appEntry.flags.blocked = blocked;
    appEntry.flags.alerted = alerted;
    appEntry.flags.is_new = isNew;
    appEntry.flags.found = 1;

    appsMap.insert(kernelPath, appEntry.v);

    return true;
}

QString ConfUtil::parseAppPath(const QStringRef &line,
                               bool &isWild, bool &isPrefix)
{
    static const QRegularExpression wildMatcher("([*?[])");

    QStringRef path = line;
    if (path.startsWith('"') && path.endsWith('"')) {
        path = path.mid(1, path.size() - 2);
    }

    if (path.isEmpty())
        return QString();

    const auto wildMatch = wildMatcher.match(path);
    if (wildMatch.hasMatch()) {
        if (wildMatch.capturedStart() == path.size() - 2
                && wildMatch.capturedRef().at(0) == '*'
                && path.endsWith('*')) {
            path.chop(2);
            isPrefix = true;
        } else {
            isWild = true;
        }
    }

    return path.toString();
}

void ConfUtil::writeData(char *output, const FirewallConf &conf,
                         const addrranges_arr_t &addressRanges,
                         const longs_arr_t &addressGroupOffsets,
                         const chars_arr_t &appPeriods,
                         quint8 appPeriodsCount,
                         const appentry_map_t &wildAppsMap,
                         const appentry_map_t &prefixAppsMap,
                         const appentry_map_t &exeAppsMap)
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

    drvConfIo->driver_version = DRIVER_VERSION;

    writeFragmentBits(&drvConfIo->conf_group.fragment_bits,
                      conf);

    writeLimits(drvConfIo->conf_group.limits,
                &drvConfIo->conf_group.limit_bits,
                &drvConfIo->conf_group.limit_2bits,
                conf.appGroups());

    writeConfFlags(conf, &drvConf->flags);

    FortCommon::confAppPermsMaskInit(drvConf);

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

void ConfUtil::writeFragmentBits(quint16 *fragmentBits,
                                 const FirewallConf &conf)
{
    *fragmentBits = 0;
    int i = 0;
    for (const AppGroup *appGroup : conf.appGroups()) {
        if (appGroup->fragmentPacket()) {
            *fragmentBits |= (1 << i);
        }
        ++i;
    }
}

void ConfUtil::writeLimits(struct fort_traf *limits,
                           quint16 *limitBits, quint32 *limit2Bits,
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

void ConfUtil::writeAddressRanges(char **data,
                                  const addrranges_arr_t &addressRanges)
{
    const int rangesCount = addressRanges.size();

    for (int i = 0; i < rangesCount; ++i) {
        const AddressRange &addressRange = addressRanges[i];

        writeAddressRange(data, addressRange);
    }
}

void ConfUtil::writeAddressRange(char **data,
                                 const AddressRange &addressRange)
{
    PFORT_CONF_ADDR_GROUP addrGroup = PFORT_CONF_ADDR_GROUP(*data);

    addrGroup->include_all = addressRange.includeAll();
    addrGroup->exclude_all = addressRange.excludeAll();

    addrGroup->include_is_empty = addressRange.includeRange().isEmpty();
    addrGroup->exclude_is_empty = addressRange.excludeRange().isEmpty();

    *data += FORT_CONF_ADDR_GROUP_OFF;

    writeAddressList(data, addressRange.includeRange());

    addrGroup->exclude_off = *data - addrGroup->data;

    writeAddressList(data, addressRange.excludeRange());
}

void ConfUtil::writeAddressList(char **data, const Ip4Range &ip4Range)
{
    PFORT_CONF_ADDR_LIST addrList = PFORT_CONF_ADDR_LIST(*data);

    addrList->ip_n = quint32(ip4Range.ipSize());
    addrList->pair_n = quint32(ip4Range.pairSize());

    *data += FORT_CONF_ADDR_LIST_OFF;

    writeLongs(data, ip4Range.ipArray());
    writeLongs(data, ip4Range.pairFromArray());
    writeLongs(data, ip4Range.pairToArray());
}

void ConfUtil::writeApps(char **data, const appentry_map_t &apps,
                         bool useHeader)
{
    quint32 *offp = (quint32 *) *data;
    const quint32 offTableSize = useHeader
            ? FORT_CONF_STR_HEADER_SIZE(apps.size()) : 0;
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
    writeNumbers(data, array.constData(), array.size(), sizeof(quint16));
}

void ConfUtil::writeLongs(char **data, const longs_arr_t &array)
{
    writeNumbers(data, array.constData(), array.size(), sizeof(quint32));
}

void ConfUtil::writeNumbers(char **data, void const *src,
                            int elemCount, uint elemSize)
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
