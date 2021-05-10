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

#include "../3rdparty/tommyds/tommyarrayof.c"
#include "../3rdparty/tommyds/tommyhash.c"
#include "../3rdparty/tommyds/tommyhashdyn.c"
#include "../3rdparty/tommyds/tommylist.c"
