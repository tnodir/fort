#include "confconstdata.h"

#include <common/fortconf.h>

ConfConstData::ConfConstData(const void *data) : m_data((const char *) data) { }

bool ConfConstData::loadAddressList(IpRange &ipRange, uint &bufSize)
{
    return loadIpRange(ipRange, bufSize)
            && (bufSize == 0 || loadIpRange(ipRange, bufSize, /*isIPv6=*/true));
}

bool ConfConstData::loadIpRange(IpRange &ipRange, uint &bufSize, bool isIPv6)
{
    if (bufSize < FORT_CONF_ADDR_LIST_OFF)
        return false;

    PFORT_CONF_ADDR_LIST addr_list = PFORT_CONF_ADDR_LIST(m_data);
    m_data = (const char *) addr_list->ip;

    const uint addrListSize = isIPv6
            ? FORT_CONF_ADDR6_LIST_SIZE(addr_list->ip_n, addr_list->pair_n)
            : FORT_CONF_ADDR4_LIST_SIZE(addr_list->ip_n, addr_list->pair_n);

    if (bufSize < addrListSize)
        return false;

    bufSize -= addrListSize;

    if (isIPv6) {
        ipRange.ip6Array().resize(addr_list->ip_n);
        ipRange.pair6FromArray().resize(addr_list->pair_n);
        ipRange.pair6ToArray().resize(addr_list->pair_n);

        loadIp6Array(ipRange.ip6Array());
        loadIp6Array(ipRange.pair6FromArray());
        loadIp6Array(ipRange.pair6ToArray());
    } else {
        ipRange.ip4Array().resize(addr_list->ip_n);
        ipRange.pair4FromArray().resize(addr_list->pair_n);
        ipRange.pair4ToArray().resize(addr_list->pair_n);

        loadLongs(ipRange.ip4Array());
        loadLongs(ipRange.pair4FromArray());
        loadLongs(ipRange.pair4ToArray());
    }

    return true;
}

void ConfConstData::loadLongs(longs_arr_t &array)
{
    loadData(array.data(), array.size(), sizeof(quint32));
}

void ConfConstData::loadIp6Array(ip6_arr_t &array)
{
    loadData(array.data(), array.size(), sizeof(ip6_addr_t));
}

void ConfConstData::loadData(void *dst, int elemCount, uint elemSize)
{
    const size_t arraySize = size_t(elemCount) * elemSize;

    memcpy(dst, m_data, arraySize);

    m_data += arraySize;
}
