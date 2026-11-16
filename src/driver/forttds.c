/* Fort Firewall TommyDS */

#include "forttds.h"

#define FORT_TOMMY_POOL_TAG 'TwfF'

FORT_API PVOID fort_tommy_malloc(SIZE_T size)
{
    return fort_mem_alloc(size, FORT_TOMMY_POOL_TAG);
}

FORT_API void fort_tommy_free(PVOID p)
{
    fort_mem_free(p, FORT_TOMMY_POOL_TAG);
}

FORT_API PVOID fort_tommy_calloc(SIZE_T count, SIZE_T element_size)
{
    const SIZE_T size = count * element_size;
    PVOID p = fort_tommy_malloc(size);

    if (p != NULL) {
        memset(p, 0, size);
    }
    return p;
}

FORT_API PVOID fort_tommy_realloc(PVOID p, SIZE_T new_size)
{
    UNUSED(p);
    UNUSED(new_size);

    NT_ASSERT(FALSE); /* not used - not implemented */

    return NULL;
}

FORT_API void fort_tommy_hashdyn_foreach_node_arg(
        tommy_hashdyn *hashdyn, tommy_foreach_arg_func *func, void *arg)
{
    tommy_hashdyn_node **bucket = hashdyn->bucket;
    const tommy_size_t bucket_max = hashdyn->bucket_max;

    for (tommy_size_t pos = 0; pos < bucket_max; ++pos) {
        tommy_hashdyn_node *node = bucket[pos];

        while (node != NULL) {
            tommy_hashdyn_node *next = node->next;
            func(arg, node);
            node = next;
        }
    }
}

#pragma warning(push)
#pragma warning(disable : 4146) /* unary minus operator applied to unsigned type */
#pragma warning(disable : 4242 4244) /* conversion, possible loss of data */

#include "../3rdparty/tommyds/tommyarrayof.c"
#include "../3rdparty/tommyds/tommyhash.c"
#include "../3rdparty/tommyds/tommyhashdyn.c"
#include "../3rdparty/tommyds/tommylist.c"

#pragma warning(pop)
