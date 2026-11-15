#ifndef PROCESSIPKEY_H
#define PROCESSIPKEY_H

#include <QHashFunctions>

#include <cstring>

#include <common/common_types.h>

// A process's remote address, compared and hashed by the bytes: the unused ones are zeroes
struct ProcessIpKey
{
    ProcessIpKey(quint32 processId, const ip_addr_t &remoteIp, bool isRemoteIPv6)
    {
        std::memset(this, 0, sizeof(ProcessIpKey));

        pid = processId;
        isIPv6 = isRemoteIPv6;

        if (isIPv6) {
            ip.v6 = remoteIp.v6;
        } else {
            ip.v4 = remoteIp.v4;
        }
    }

    friend bool operator==(const ProcessIpKey &a, const ProcessIpKey &b)
    {
        return std::memcmp(&a, &b, sizeof(ProcessIpKey)) == 0;
    }

    friend size_t qHash(const ProcessIpKey &key, size_t seed = 0)
    {
        return qHashBits(&key, sizeof(ProcessIpKey), seed);
    }

    quint32 pid;
    quint32 isIPv6;
    ip_addr_t ip;
};

#endif // PROCESSIPKEY_H
