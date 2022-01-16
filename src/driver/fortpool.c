/* Fort Firewall Pool List */

#include "fortpool.h"

#define FORT_POOL_SIZE     (64 * 1024)
#define FORT_POOL_OVERHEAD 256
#define FORT_POOL_SIZE_MIN (FORT_POOL_SIZE - FORT_POOL_OVERHEAD)
#define FORT_POOL_SIZE_MAX (TLSF_MAX_POOL_SIZE - FORT_POOL_OVERHEAD)

#define fort_pool_size(size)                                                                       \
    ((size) < FORT_POOL_SIZE_MIN ? FORT_POOL_SIZE                                                  \
                                 : ((size) < (FORT_POOL_SIZE_MAX / 2) ? 2 * (size) : (size)))

static tommy_node *fort_pool_new(UINT32 pool_size)
{
    if (pool_size > FORT_POOL_SIZE_MAX)
        return NULL;

    return tommy_malloc(pool_size);
}

static void fort_pool_del(tommy_node *pool)
{
    tommy_free(pool);
}

FORT_API void fort_pool_list_init(PFORT_POOL_LIST pool_list)
{
    tommy_list_init(&pool_list->pools);
}

FORT_API void fort_pool_init(PFORT_POOL_LIST pool_list, UINT32 size)
{
    const UINT32 pool_size = fort_pool_size(size);

    tommy_node *pool = fort_pool_new(pool_size);
    if (pool == NULL)
        return;

    tommy_list_insert_first(&pool_list->pools, pool);

    pool_list->tlsf = tlsf_create_with_pool(
            (char *) pool + FORT_POOL_DATA_OFF, pool_size - FORT_POOL_DATA_OFF);
}

FORT_API void fort_pool_done(PFORT_POOL_LIST pool_list)
{
    tommy_node *pool = tommy_list_head(&pool_list->pools);
    while (pool != NULL) {
        tommy_node *next = pool->next;
        fort_pool_del(pool);
        pool = next;
    }
}

FORT_API void *fort_pool_malloc(PFORT_POOL_LIST pool_list, UINT32 size)
{
    tommy_node *pool = tommy_list_tail(&pool_list->pools);

    if (pool == NULL)
        return NULL;

    void *p = tlsf_malloc(pool_list->tlsf, size);
    if (p == NULL) {
        const UINT32 pool_size = fort_pool_size(size);

        pool = fort_pool_new(pool_size);
        if (pool == NULL)
            return NULL;

        tommy_list_insert_head_not_empty(&pool_list->pools, pool);

        tlsf_add_pool(pool_list->tlsf, (char *) pool + FORT_POOL_DATA_OFF,
                pool_size - FORT_POOL_DATA_OFF);

        p = tlsf_malloc(pool_list->tlsf, size);
    }

    return p;
}

FORT_API void fort_pool_free(PFORT_POOL_LIST pool_list, void *p)
{
    tlsf_free(pool_list->tlsf, p);
}
