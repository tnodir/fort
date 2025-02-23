#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

enum {
    IpProto_ICMP = 1,
    IpProto_IGMP = 2,
    IpProto_TCP = 6,
    IpProto_UDP = 17,
    IpProto_ICMPV6 = 58,
};

typedef struct ip6_addr_t
{
    union {
        char data[16];
        unsigned short addr16[8];
        unsigned int addr32[4];
        struct
        {
            unsigned long long lo64;
            unsigned long long hi64;
        };
    };
} ip6_addr_t;

typedef union ip_addr_t {
    unsigned short v2;
    unsigned int v4;
    ip6_addr_t v6;
    char data[sizeof(ip6_addr_t)];
} ip_addr_t;

typedef struct fort_app_path
{
    unsigned short len;
    const void *buffer;
} FORT_APP_PATH, *PFORT_APP_PATH;

typedef const FORT_APP_PATH *PCFORT_APP_PATH;

typedef struct fort_conf_rule_zones
{
    unsigned int accept_mask;
    unsigned int reject_mask;
} FORT_CONF_RULE_ZONES, *PFORT_CONF_RULE_ZONES;

typedef const FORT_CONF_RULE_ZONES *PCFORT_CONF_RULE_ZONES;

#define UNUSED(p) ((void) (p))

#endif // COMMON_TYPES_H
