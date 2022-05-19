#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

typedef struct ip6_addr_t
{
    unsigned short word[8];
} ip6_addr_t;

#define UNUSED(p) ((void) (p))

#endif // COMMON_TYPES_H
