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
#include "../dateutil.h"
#include "../fileutil.h"
#include "../net/ip4range.h"

#define APP_GROUP_MAX       FORT_CONF_GROUP_MAX
#define APP_GROUP_NAME_MAX  128
#define APP_PATH_MAX        (FORT_CONF_APP_PATH_MAX / sizeof(wchar_t))

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

int ConfUtil::write(const FirewallConf &conf, QByteArray &buf)
{
    quint32 addressGroupsSize = 0;
    longs_arr_t addressGroupOffsets;
    addrranges_arr_t addressRanges(conf.addressGroupsList().size());

    if (!parseAddressGroups(conf.addressGroupsList(), addressRanges,
                            addressGroupOffsets, addressGroupsSize))
        return false;

    quint32 appPathsLen = 0;
    QStringList appPaths;
    longs_arr_t appPerms;
    quint8 appPeriodsCount = 0;
    chars_arr_t appPeriods;
    appgroups_map_t appGroupIndexes;

    if (!parseAppGroups(conf.appGroupsList(),
                        appPaths, appPathsLen, appPerms,
                        appPeriods, appPeriodsCount, appGroupIndexes))
        return false;

    if (appPathsLen > FORT_CONF_APPS_LEN_MAX) {
        setErrorMessage(tr("Too many application paths"));
        return false;
    }

    // Fill the buffer
    const int confIoSize = FORT_CONF_IO_CONF_OFF + FORT_CONF_DATA_OFF
            + addressGroupsSize
            + FORT_CONF_STR_DATA_SIZE(appGroupIndexes.size())  // appPerms
            + FORT_CONF_STR_DATA_SIZE(conf.appGroupsList().size()
                                      * sizeof(FORT_PERIOD))  // appPeriods
            + appPaths.size() * sizeof(quint32)
            + FORT_CONF_STR_HEADER_SIZE(appPaths.size())
            + FORT_CONF_STR_DATA_SIZE(appPathsLen);

    buf.reserve(confIoSize);

    writeData(buf.data(), conf,
              addressRanges, addressGroupOffsets,
              appPaths, appPerms,
              appPeriods, appPeriodsCount, appGroupIndexes);

    return confIoSize;
}

int ConfUtil::writeFlags(const FirewallConf &conf, QByteArray &buf)
{
    const int flagsSize = sizeof(FORT_CONF_FLAGS);

    buf.reserve(flagsSize);

    // Fill the buffer
    PFORT_CONF_FLAGS confFlags = (PFORT_CONF_FLAGS) buf.data();

    confFlags->prov_boot = conf.provBoot();
    confFlags->filter_enabled = conf.filterEnabled();
    confFlags->filter_locals = conf.filterLocals();
    confFlags->stop_traffic = conf.stopTraffic();
    confFlags->stop_inet_traffic = conf.stopInetTraffic();
    confFlags->app_block_all = conf.appBlockAll();
    confFlags->app_allow_all = conf.appAllowAll();
    confFlags->log_blocked = conf.logBlocked();
    confFlags->log_stat = conf.logStat();
    confFlags->group_bits = conf.appGroupBits();

    return flagsSize;
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

        const int incRangeSize = addressRange.includeRange().size();
        const int excRangeSize = addressRange.excludeRange().size();

        if (incRangeSize > FORT_CONF_IP_MAX
                || excRangeSize > FORT_CONF_IP_MAX) {
            setErrorMessage(tr("Too many IP addresses"));
            return false;
        }

        addressGroupOffsets.append(addressGroupsSize);

        addressGroupsSize += FORT_CONF_ADDR_DATA_OFF
                + FORT_CONF_IP_RANGE_SIZE(incRangeSize)
                + FORT_CONF_IP_RANGE_SIZE(excRangeSize);
    }

    return true;
}

bool ConfUtil::parseAppGroups(const QList<AppGroup *> &appGroups,
                              QStringList &appPaths,
                              quint32 &appPathsLen,
                              longs_arr_t &appPerms,
                              chars_arr_t &appPeriods,
                              quint8 &appPeriodsCount,
                              appgroups_map_t &appGroupIndexes)
{
    const int groupsCount = appGroups.size();
    if (groupsCount > APP_GROUP_MAX) {
        setErrorMessage(tr("Number of Application Groups must be < %1")
                        .arg(APP_GROUP_MAX));
        return false;
    }

    appperms_map_t appPermsMap;

    for (int i = 0; i < groupsCount; ++i) {
        const AppGroup *appGroup = appGroups.at(i);

        const QString name = appGroup->name();
        if (name.size() > APP_GROUP_NAME_MAX) {
            setErrorMessage(tr("Length of Application Group's Name must be < %1")
                            .arg(APP_GROUP_NAME_MAX));
            return false;
        }

        if (!parseApps(appGroup->blockText(), true,
                       appPermsMap, appGroupIndexes, i)
                || !parseApps(appGroup->allowText(), false,
                              appPermsMap, appGroupIndexes, i))
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

    // Fill app. paths & perms arrays
    {
        appPerms.reserve(appPermsMap.size());

        auto it = appPermsMap.constBegin();
        auto end = appPermsMap.constEnd();
        for (; it != end; ++it) {
            const QString &appPath = it.key();
            appPathsLen += quint32(appPath.size()) * sizeof(wchar_t);

            appPaths.append(appPath);
            appPerms.append(it.value());
        }
    }

    return true;
}

bool ConfUtil::parseApps(const QString &text, bool blocked,
                         appperms_map_t &appPermsMap,
                         appgroups_map_t &appGroupIndexes,
                         int groupOffset)
{
    foreach (const QStringRef &line,
             text.splitRef(QLatin1Char('\n'))) {
        const QStringRef lineTrimmed = line.trimmed();
        if (lineTrimmed.isEmpty()
                || lineTrimmed.startsWith('#'))  // commented line
            continue;

        const QString appPath = parseAppPath(lineTrimmed);
        if (appPath.isEmpty())
            continue;

        if (appPath.size() > int(APP_PATH_MAX)) {
            setErrorMessage(tr("Length of Application's Path must be < %1")
                            .arg(APP_PATH_MAX));
            return false;
        }

        const quint32 permVal = blocked ? 2 : 1;
        const quint32 appPerm = permVal << (groupOffset * 2);
        quint32 appPerms = appPerm;

        if (appPermsMap.contains(appPath)) {
            appPerms |= appPermsMap.value(appPath);
        } else {
            appGroupIndexes.insert(appPath, qint8(groupOffset));
        }

        appPermsMap.insert(appPath, appPerms);
    }

    return true;
}

QString ConfUtil::parseAppPath(const QStringRef &line)
{
    const QRegularExpression re(R"(\s*"?\s*([^"]+)\s*"?\s*)");
    const QRegularExpressionMatch match = re.match(line);

    if (!match.hasMatch())
        return QString();

    const QStringRef path = match.capturedRef(1).trimmed();

    const QString systemPath("System");
    if (!QStringRef::compare(path, systemPath, Qt::CaseInsensitive))
        return systemPath;

    const QString kernelPath = FileUtil::pathToKernelPath(path.toString());
    return kernelPath.toLower();
}

void ConfUtil::writeData(char *output, const FirewallConf &conf,
                         const addrranges_arr_t &addressRanges,
                         const longs_arr_t &addressGroupOffsets,
                         const QStringList &appPaths,
                         const longs_arr_t &appPerms,
                         const chars_arr_t &appPeriods,
                         quint8 appPeriodsCount,
                         const appgroups_map_t &appGroupIndexes)
{
    PFORT_CONF_IO drvConfIo = (PFORT_CONF_IO) output;
    PFORT_CONF drvConf = &drvConfIo->conf;
    char *data = drvConf->data;
    const quint32 appPathsSize = quint32(appPaths.size());
    quint32 addrGroupsOff, appGroupsOff;
    quint32 appPathsOff, appPermsOff, appPeriodsOff;

#define CONF_DATA_OFFSET quint32(data - drvConf->data)
    addrGroupsOff = CONF_DATA_OFFSET;
    writeLongs(&data, addressGroupOffsets);
    writeAddressRanges(&data, addressRanges);

    appGroupsOff = CONF_DATA_OFFSET;
    writeChars(&data, appGroupIndexes.values().toVector());

    appPermsOff = CONF_DATA_OFFSET;
    writeLongs(&data, appPerms);

    appPeriodsOff = CONF_DATA_OFFSET;
    writeChars(&data, appPeriods);

    appPathsOff = CONF_DATA_OFFSET;
    writeStrings(&data, appPaths);
#undef CONF_DATA_OFFSET

    drvConfIo->driver_version = DRIVER_VERSION;

    writeFragmentBits(&drvConfIo->conf_group.fragment_bits,
                      conf);

    writeLimits(drvConfIo->conf_group.limits,
                &drvConfIo->conf_group.limit_bits,
                &drvConfIo->conf_group.limit_2bits,
                conf.appGroupsList());

    drvConf->flags.prov_boot = conf.provBoot();
    drvConf->flags.filter_enabled = conf.filterEnabled();
    drvConf->flags.filter_locals = conf.filterLocals();
    drvConf->flags.stop_traffic = conf.stopTraffic();
    drvConf->flags.stop_inet_traffic = conf.stopInetTraffic();

    drvConf->flags.app_block_all = conf.appBlockAll();
    drvConf->flags.app_allow_all = conf.appAllowAll();

    drvConf->flags.log_blocked = conf.logBlocked();
    drvConf->flags.log_stat = conf.logStat();

    drvConf->flags.group_bits = conf.appGroupBits();

    FortCommon::confAppPermsMaskInit(drvConf);

    drvConf->apps_n = quint16(appPathsSize);
    drvConf->app_periods_n = appPeriodsCount;

    drvConf->addr_groups_off = addrGroupsOff;

    drvConf->app_groups_off = appGroupsOff;
    drvConf->app_perms_off = appPermsOff;
    drvConf->app_periods_off = appPeriodsOff;
    drvConf->apps_off = appPathsOff;
}

void ConfUtil::writeFragmentBits(quint16 *fragmentBits,
                                 const FirewallConf &conf)
{
    *fragmentBits = 0;
    int i = 0;
    foreach (const AppGroup *appGroup, conf.appGroupsList()) {
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

    addrGroup->include_n = quint32(addressRange.includeRange()
                                   .fromArray().size());
    addrGroup->exclude_n = quint32(addressRange.excludeRange()
                                   .fromArray().size());

    *data += FORT_CONF_ADDR_DATA_OFF;

    writeLongs(data, addressRange.includeRange().fromArray());
    writeLongs(data, addressRange.includeRange().toArray());

    writeLongs(data, addressRange.excludeRange().fromArray());
    writeLongs(data, addressRange.excludeRange().toArray());
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

void ConfUtil::writeStrings(char **data, const QStringList &list)
{
    quint32 *offp = (quint32 *) *data;
    const quint32 offTableSize = FORT_CONF_STR_HEADER_SIZE(list.size());
    char *p = *data + offTableSize;
    quint32 off = 0;

    *offp++ = 0;

    foreach (const QString &s, list) {
        const quint32 len = s.toWCharArray((wchar_t *) p)
                * sizeof(wchar_t);

        off += len;
        *offp++ = off;
        p += len;
    }

    *data += offTableSize + FORT_CONF_STR_DATA_SIZE(off);
}
