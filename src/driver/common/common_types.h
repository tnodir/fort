#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

typedef struct ip6_addr_t
{
    union {
        char data[16];
        struct
        {
            unsigned int part1;
            unsigned int part2;
            unsigned int part3;
            unsigned int part4;
        };
        struct
        {
            unsigned long long lo;
            unsigned long long hi;
        };
    };
} ip6_addr_t;

typedef union ip_addr_t {
    unsigned int v4;
    ip6_addr_t v6;
} ip_addr_t;

#define UNUSED(p) ((void) (p))

#endif // COMMON_TYPES_H
