#ifndef FORTPOOL_H
#define FORTPOOL_H

#include "fortdrv.h"

#include "forttds.h"
#include "forttlsf.h"

#define FORT_POOL_DATA_OFF offsetof(FORT_POOL, data)

/* Synchronize with tommy_node! */
typedef struct fort_pool
{
    struct fort_pool *next;
    struct fort_pool *prev;

    char data[4];
} FORT_POOL, *PFORT_POOL;

typedef struct fort_pool_list
{
    tlsf_t tlsf;
    tommy_list pools;
} FORT_POOL_LIST, *PFORT_POOL_LIST;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_pool_list_init(PFORT_POOL_LIST pool_list);

FORT_API void fort_pool_init(PFORT_POOL_LIST pool_list, UINT32 size);

FORT_API void fort_pool_done(PFORT_POOL_LIST pool_list);

FORT_API void *fort_pool_malloc(PFORT_POOL_LIST pool_list, UINT32 size);

FORT_API void fort_pool_free(PFORT_POOL_LIST pool_list, void *p);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPOOL_H
