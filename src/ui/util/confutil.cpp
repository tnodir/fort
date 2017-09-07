#include "confutil.h"

#include <QRegularExpression>

#define UCHAR   quint8
#define UINT16  quint16
#define UINT32  quint32

#include "../common/fortconf.h"
#include "../conf/addressgroup.h"
#include "../conf/appgroup.h"
#include "../conf/firewallconf.h"
#include "../fortcommon.h"
#include "fileutil.h"
#include "ip4range.h"

#define APP_GROUP_MAX       16
#define APP_GROUP_NAME_MAX  128
#define APP_PATH_MAX        1024

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
    Ip4Range incRange;
    if (!incRange.fromText(conf.ipInclude()->text())) {
        setErrorMessage(tr("Bad Include IP address: %1")
                        .arg(incRange.errorLineAndMessage()));
        return false;
    }

    Ip4Range excRange;
    if (!excRange.fromText(conf.ipExclude()->text())) {
        setErrorMessage(tr("Bad Exclude IP address: %1")
                        .arg(excRange.errorLineAndMessage()));
        return false;
    }

    int groupNamesLen = 0;
    int appPathsLen = 0;
    QStringList groupNames;
    QStringList appPaths;
    appperms_arr_t appPerms;

    if (!parseAppGroups(conf.appGroupsList(),
                        groupNames, groupNamesLen,
                        appPaths, appPathsLen, appPerms))
        return false;

    // Calculate maximum required buffer size
    if (incRange.size() > FORT_CONF_IP_MAX
            || excRange.size() > FORT_CONF_IP_MAX
            || groupNamesLen > FORT_CONF_GROUPS_LEN_MAX
            || appPathsLen > FORT_CONF_APPS_LEN_MAX) {
        setErrorMessage(tr("Size of configuration is too big"));
        return false;
    }

    // Fill the buffer
    const int confSize = FORT_CONF_DATA_OFF
            + (incRange.size() + excRange.size()) * 2 * sizeof(quint32)
            + appPaths.size() * sizeof(quint32)
            + FORT_CONF_STR_HEADER_SIZE(appPaths.size())
            + FORT_CONF_STR_DATA_SIZE(appPathsLen)
            + FORT_CONF_STR_HEADER_SIZE(groupNames.size())
            + FORT_CONF_STR_DATA_SIZE(groupNamesLen);

    buf.reserve(confSize);

    writeData(buf.data(), conf,
              incRange, excRange,
              groupNames, appPaths,
              appPerms);

    return confSize;
}

int ConfUtil::writeFlags(const FirewallConf &conf, QByteArray &buf)
{
    const int flagsSize = sizeof(FORT_CONF_FLAGS);

    buf.reserve(flagsSize);

    // Fill the buffer
    PFORT_CONF_FLAGS confFlags = (PFORT_CONF_FLAGS) buf.data();

    confFlags->filter_enabled = conf.filterEnabled();
    confFlags->ip_include_all = conf.ipInclude()->useAll();
    confFlags->ip_exclude_all = conf.ipExclude()->useAll();
    confFlags->app_log_blocked = conf.appLogBlocked();
    confFlags->app_block_all = conf.appBlockAll();
    confFlags->app_allow_all = conf.appAllowAll();
    confFlags->group_bits = conf.appGroupBits();

    return flagsSize;
}

bool ConfUtil::parseAppGroups(const QList<AppGroup *> &appGroups,
                              QStringList &groupNames,
                              int &groupNamesLen,
                              QStringList &appPaths,
                              int &appPathsLen,
                              appperms_arr_t &appPerms)
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

        groupNames.append(name);
        groupNamesLen += name.size() * sizeof(wchar_t);

        if (!parseApps(appGroup->blockText(), true, appPermsMap, i)
                || !parseApps(appGroup->allowText(), false, appPermsMap, i))
            return false;
    }

    // Fill app. paths & perms arrays
    {
        appperms_map_t::const_iterator it = appPermsMap.constBegin();
        appperms_map_t::const_iterator end = appPermsMap.constEnd();

        appPerms.reserve(appPermsMap.size());

        for (; it != end; ++it) {
            const QString appPath = it.key();
            appPathsLen += appPath.size() * sizeof(wchar_t);

            appPaths.append(appPath);
            appPerms.append(it.value());
        }
    }

    return true;
}

bool ConfUtil::parseApps(const QString &text, bool blocked,
                         appperms_map_t &appPermsMap, int groupOffset)
{
    foreach (const QStringRef &line,
             text.splitRef(QLatin1Char('\n'))) {
        if (line.isEmpty())
            continue;

        const QString appPath = parseAppPath(line);
        if (appPath.isEmpty())
            continue;

        if (appPath.size() > APP_PATH_MAX) {
            setErrorMessage(tr("Length of Application's Path must be < %1")
                            .arg(APP_PATH_MAX));
            return false;
        }

        const quint32 permVal = blocked ? 2 : 1;
        const quint32 appPerm = permVal << (groupOffset * 2);
        const quint32 appPerms = appPerm | appPermsMap.value(appPath, 0);

        appPermsMap.insert(appPath, appPerms);
    }

    return true;
}

QString ConfUtil::parseAppPath(const QStringRef &line)
{
    const QRegularExpression re("\\s*\"?\\s*(\\S+)\\s*\"?\\s*");
    const QRegularExpressionMatch match = re.match(line);

    if (!match.hasMatch())
        return QString();

    const QStringRef path = match.capturedRef(1);

    const QString systemPath("System");
    if (!QStringRef::compare(path, systemPath, Qt::CaseInsensitive))
        return systemPath;

    const QString dosPath = FileUtil::pathToDosPath(path.toString());
    return dosPath.toLower();
}

void ConfUtil::writeData(char *output, const FirewallConf &conf,
                         const Ip4Range &incRange, const Ip4Range &excRange,
                         const QStringList &groupNames, const QStringList &appPaths,
                         const appperms_arr_t &appPerms)
{
    PFORT_CONF drvConf = (PFORT_CONF) output;
    char *data = (char *) &drvConf->data;
    const quint32 incRangeSize = incRange.size();
    const quint32 excRangeSize = excRange.size();
    const quint32 groupNamesSize = groupNames.size();
    const quint32 appPathsSize = appPaths.size();
    quint32 incRangeFromOff, incRangeToOff;
    quint32 excRangeFromOff, excRangeToOff;
    quint32 groupNamesOff, appPathsOff, appPermsOff;

#define CONF_DATA_OFFSET (data - (char *) &drvConf->data)
    incRangeFromOff = CONF_DATA_OFFSET;
    writeNumbers(&data, incRange.fromArray());

    incRangeToOff = CONF_DATA_OFFSET;
    writeNumbers(&data, incRange.toArray());

    excRangeFromOff = CONF_DATA_OFFSET;
    writeNumbers(&data, excRange.fromArray());

    excRangeToOff = CONF_DATA_OFFSET;
    writeNumbers(&data, excRange.toArray());

    appPermsOff = CONF_DATA_OFFSET;
    writeNumbers(&data, appPerms);

    appPathsOff = CONF_DATA_OFFSET;
    writeStrings(&data, appPaths);

    groupNamesOff = CONF_DATA_OFFSET;
    writeStrings(&data, groupNames);
#undef CONF_DATA_OFFSET

    drvConf->flags.filter_enabled = conf.filterEnabled();

    drvConf->flags.ip_include_all = conf.ipInclude()->useAll();
    drvConf->flags.ip_exclude_all = conf.ipExclude()->useAll();

    drvConf->flags.app_log_blocked = conf.appLogBlocked();
    drvConf->flags.app_block_all = conf.appBlockAll();
    drvConf->flags.app_allow_all = conf.appAllowAll();

    drvConf->flags.group_bits = conf.appGroupBits();

    FortCommon::confAppPermsMaskInit(drvConf);

    drvConf->conf_version = FORT_CONF_VERSION;
    drvConf->data_off = FORT_CONF_DATA_OFF;

    drvConf->ip_include_n = incRangeSize;
    drvConf->ip_exclude_n = excRangeSize;

    drvConf->apps_n = appPathsSize;
    drvConf->groups_n = groupNamesSize;

    drvConf->ip_from_include_off = incRangeFromOff;
    drvConf->ip_to_include_off = incRangeToOff;

    drvConf->ip_from_exclude_off = excRangeFromOff;
    drvConf->ip_to_exclude_off = excRangeToOff;

    drvConf->app_perms_off = appPermsOff;
    drvConf->apps_off = appPathsOff;
    drvConf->groups_off = groupNamesOff;
}

void ConfUtil::writeNumbers(char **data, const QVector<quint32> &array)
{
    const int arraySize = array.size() * sizeof(quint32);

    memcpy(*data, array.constData(), arraySize);

    *data += arraySize;
}

void ConfUtil::writeStrings(char **data, const QStringList &list)
{
    quint32 *offp = (quint32 *) *data;
    const quint32 offTableSize = FORT_CONF_STR_HEADER_SIZE(list.size());
    char *p = *data + offTableSize;
    quint32 off = 0;

    *offp++ = 0;

    foreach (const QString &s, list) {
        const int len = s.toWCharArray((wchar_t *) p)
                * sizeof(wchar_t);

        off += len;
        *offp++ = off;
        p += len;
    }

    *data += offTableSize + FORT_CONF_STR_DATA_SIZE(off);
}
