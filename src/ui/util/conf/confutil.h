#ifndef CONFUTIL_H
#define CONFUTIL_H

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QRegularExpressionMatch>
#include <QVector>

#include <util/service/serviceinfo.h>

#include "appparseoptions.h"

class FirewallConf;

using longs_arr_t = QVector<quint32>;
using shorts_arr_t = QVector<quint16>;
using chars_arr_t = QVector<qint8>;

struct WriteServiceSidsArgs
{
    QMap<QByteArray, int> sidNameIndexMap;
    QStringList namesList;
};

struct ParseAddressGroupsArgs
{
    addrranges_arr_t addressRanges;
    longs_arr_t addressGroupOffsets;
};

struct WriteConfArgs
{
    const FirewallConf &conf;

    ParseAddressGroupsArgs ad;
};

class ConfUtil
{
public:
    static int ruleMaxCount();
    static int ruleSetMaxCount();
    static int ruleDepthMaxCount();
    static int ruleSetDepthMaxCount();

    static int zoneMaxCount();

    static QRegularExpressionMatch matchWildcard(const QStringView &path);

    static int writeServiceSids(char **data, const WriteServiceSidsArgs &wssa);

    static QString parseAppPath(const QStringView &line, bool &isWild, bool &isPrefix);

    static void writeConf(char **data, const WriteConfArgs &wca, AppParseOptions &opt);
    static void writeConfFlags(char **data, const FirewallConf &conf);

    static void writeAddressRanges(char **data, const addrranges_arr_t &addressRanges);
    static void writeAddressRange(char **data, const AddressRange &addressRange);

    static void writeAddressList(char **data, const IpRange &ipRange);
    static void writeIpRange(char **data, const IpRange &ipRange, bool isIPv6 = false);

    static bool loadAddressList(const char **data, IpRange &ipRange, uint &bufSize);
    static bool loadIpRange(
            const char **data, IpRange &ipRange, uint &bufSize, bool isIPv6 = false);

    static void writeApps(char **data, const appdata_map_t &appsMap, bool useHeader = false);

    static void migrateZoneData(char **data, const QByteArray &zoneData);

    static void writeShorts(char **data, const shorts_arr_t &array);
    static void writeLongs(char **data, const longs_arr_t &array);
    static void writeIp6Array(char **data, const ip6_arr_t &array);
    static void writeData(char **data, void const *src, int elemCount, uint elemSize);
    static void writeChars(char **data, const chars_arr_t &array);
    static void writeArray(char **data, const QByteArray &array);
    static void writeString(char **data, const QString &s);

    static void loadLongs(const char **data, longs_arr_t &array);
    static void loadIp6Array(const char **data, ip6_arr_t &array);
    static void loadData(const char **data, void *dst, int elemCount, uint elemSize);
};

#endif // CONFUTIL_H
