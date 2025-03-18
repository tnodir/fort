#ifndef CONFDATA_H
#define CONFDATA_H

#include <QByteArray>
#include <QObject>

#include <util/service/serviceinfo.h>

#include "appparseoptions.h"
#include "conf_types.h"

class ActionRange;
class AreaRange;
class DirRange;
class FirewallConf;
class PortRange;
class ProfileRange;
class ProtoRange;
class ValueRange;

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
    char *base() const { return m_base; }

    quint32 dataOffset() const { return m_data - m_base; }
    void resetBase() { m_base = m_data; }

    void writeConf(const WriteConfArgs &wca, AppParseOptions &opt);
    void writeConfFlags(const FirewallConf &conf);

    void writeAddressRanges(const addrranges_arr_t &addressRanges);
    void writeAddressRange(const AddressRange &addressRange);

    void writeAddressList(const IpRange &ipRange);
    void writeIpRange(const IpRange &ipRange, bool isIPv6 = false);

    void writePortRange(const PortRange &portRange);
    void writeProtoRange(const ProtoRange &protoRange);
    void writeDirRange(const DirRange &dirRange);
    void writeAreaRange(const AreaRange &areaRange);
    void writeProfileRange(const ProfileRange &profileRange);
    void writeActionRange(const ActionRange &actionRange);

    void writeApps(const appdata_map_t &appsMap, bool useHeader = false);

    void migrateZoneData(const QByteArray &zoneData);

    void writeBytes(const bytes_arr_t &array);
    void writeShorts(const shorts_arr_t &array);
    void writeLongs(const longs_arr_t &array);
    void writeIp6Array(const ip6_arr_t &array);
    void writeData(void const *src, int elemCount, uint elemSize);
    void writeChars(const chars_arr_t &array);
    void writeArray(const QByteArray &array);
    void writeString(const QString &s);

protected:
    char *m_data = nullptr;
    char *m_base = nullptr;
};

#endif // CONFDATA_H
