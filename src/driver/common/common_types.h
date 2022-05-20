#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

typedef struct ip6_addr_t
{
    char data[16];
} ip6_addr_t;

#define UNUSED(p) ((void) (p))

#endif // COMMON_TYPES_H
