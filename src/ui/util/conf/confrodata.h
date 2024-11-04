#ifndef CONFRODATA_H
#define CONFRODATA_H

#include <QObject>

#include <util/net/iprange.h>

#include "conf_types.h"

class ConfRoData
{
public:
    explicit ConfRoData(const void *data);

    bool loadAddressList(IpRange &ipRange, uint &bufSize);
    bool loadIpRange(IpRange &ipRange, uint &bufSize, bool isIPv6 = false);

    void loadLongs(longs_arr_t &array);
    void loadIp6Array(ip6_arr_t &array);
    void loadData(void *dst, int elemCount, uint elemSize);

private:
    const char *m_data = nullptr;
};

#endif // CONFRODATA_H
