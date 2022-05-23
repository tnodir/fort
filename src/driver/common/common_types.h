#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

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
    unsigned int v4;
    ip6_addr_t v6;
} ip_addr_t;

#define UNUSED(p) ((void) (p))

#endif // COMMON_TYPES_H
