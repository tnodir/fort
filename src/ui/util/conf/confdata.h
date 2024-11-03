#ifndef CONFDATA_H
#define CONFDATA_H

#include <QByteArray>
#include <QObject>

#include <util/service/serviceinfo.h>

#include "appparseoptions.h"
#include "conf_types.h"

class FirewallConf;

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

class ConfData
{
public:
    explicit ConfData(void *data);

    char *data() const { return m_data; }

    void writeConf(const WriteConfArgs &wca, AppParseOptions &opt);
    void writeConfFlags(const FirewallConf &conf);

    void writeAddressRanges(const addrranges_arr_t &addressRanges);
    void writeAddressRange(const AddressRange &addressRange);

    void writeAddressList(const IpRange &ipRange);
    void writeIpRange(const IpRange &ipRange, bool isIPv6 = false);

    bool loadAddressList(IpRange &ipRange, uint &bufSize);
    bool loadIpRange(IpRange &ipRange, uint &bufSize, bool isIPv6 = false);

    void writeApps(const appdata_map_t &appsMap, bool useHeader = false);

    void migrateZoneData(const QByteArray &zoneData);

    void writeShorts(const shorts_arr_t &array);
    void writeLongs(const longs_arr_t &array);
    void writeIp6Array(const ip6_arr_t &array);
    void writeData(void const *src, int elemCount, uint elemSize);
    void writeChars(const chars_arr_t &array);
    void writeArray(const QByteArray &array);
    void writeString(const QString &s);

private:
    char *m_data = nullptr;
};

#endif // CONFDATA_H
