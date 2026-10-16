/* Fort Firewall Processes' Names */

#include "fortpsname.h"

#include "common/fortcmnutl.h"

#define FORT_PSNAMES_POOL_SIZE (4 * 1024)

#define FORT_PSNAME_NODE_DATA_OFF offsetof(FORT_PSNAME_NODE, data)

#define fort_psname_hash(path) tommy_hash_u32(0, (path)->buffer, (path)->len)

FORT_API void fort_psname_open(PFORT_PSNAMES ps_names)
{
    fort_pool_list_init(&ps_names->pool_list);
    fort_pool_init(&ps_names->pool_list, FORT_PSNAMES_POOL_SIZE);

    tommy_hashdyn_init(&ps_names->names_map);

    KeInitializeSpinLock(&ps_names->lock);
}

FORT_API void fort_psname_close(PFORT_PSNAMES ps_names)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&ps_names->lock, &lock_queue);
    {
        fort_pool_done(&ps_names->pool_list);

        tommy_hashdyn_done(&ps_names->names_map);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static int fort_psname_cmp(const void *arg, const void *obj)
{
    PCFORT_APP_PATH path = arg;
    PCFORT_PSNAME_NODE ps_name = obj;

    if (ps_name->size != path->len)
        return 1;

    return !fort_mem_eql(ps_name->data, path->buffer, path->len);
}

static PFORT_PSNAME_NODE fort_psname_new_locked(
        PFORT_PSNAMES ps_names, PCFORT_APP_PATH path, tommy_hash_t hash)
{
    const UINT16 size = path->len;

    PFORT_PSNAME_NODE ps_name = fort_pool_malloc(&ps_names->pool_list,
            FORT_PSNAME_NODE_DATA_OFF + size + sizeof(WCHAR)); /* include terminating zero */
    if (ps_name == NULL)
        return NULL;

    ps_name->refcount = 1;
    ps_name->size = size;

    RtlCopyMemory(ps_name->data, path->buffer, size);
    ps_name->data[size / sizeof(WCHAR)] = L'\0';

    tommy_hashdyn_insert(&ps_names->names_map, &ps_name->node, ps_name, hash);

    return ps_name;
}

static PFORT_PSNAME_NODE fort_psname_get_locked(
        PFORT_PSNAMES ps_names, PCFORT_APP_PATH path, tommy_hash_t hash)
{
    PFORT_PSNAME_NODE ps_name =
            tommy_hashdyn_search(&ps_names->names_map, &fort_psname_cmp, path, hash);

    if (ps_name == NULL) {
        return fort_psname_new_locked(ps_names, path, hash);
    }

    /* Not released meanwhile: the last reference is put under the lock */
    fort_psname_ref(ps_name);

    return ps_name;
}

FORT_API PFORT_PSNAME_NODE fort_psname_get(PFORT_PSNAMES ps_names, PCFORT_APP_PATH path)
{
    const tommy_hash_t hash = fort_psname_hash(path);

    PFORT_PSNAME_NODE ps_name;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&ps_names->lock, &lock_queue);
    {
        ps_name = fort_psname_get_locked(ps_names, path, hash);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return ps_name;
}

FORT_API void fort_psname_ref(PFORT_PSNAME_NODE ps_name)
{
    /* The caller's reference keeps it */
    InterlockedIncrement(&ps_name->refcount);
}

FORT_API void fort_psname_put(PFORT_PSNAMES ps_names, PFORT_PSNAME_NODE ps_name)
{
    if (ps_name == NULL)
        return;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&ps_names->lock, &lock_queue);
    {
        if (InterlockedDecrement(&ps_name->refcount) == 0) {
            tommy_hashdyn_remove_existing(&ps_names->names_map, &ps_name->node);

            fort_pool_free(&ps_names->pool_list, ps_name);
        }
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);
}
