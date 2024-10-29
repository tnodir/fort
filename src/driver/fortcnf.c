/* Fort Firewall Configuration */

#include "fortcnf.h"

#define FORT_DEVICE_CONF_POOL_TAG 'CwfF'

/* Synchronize with tommy_hashdyn_node! */
typedef struct fort_conf_exe_node
{
    struct fort_conf_exe_node *next;
    struct fort_conf_exe_node *prev;

    PFORT_APP_ENTRY app_entry; /* tommy_hashdyn_node::data */

    tommy_key_t path_hash; /* tommy_hashdyn_node::index */
} FORT_CONF_EXE_NODE, *PFORT_CONF_EXE_NODE;

static int bit_scan_forward(ULONG mask)
{
    unsigned long index;
    return _BitScanForward(&index, mask) ? index : -1;
}

static PVOID fort_conf_mem_alloc(PVOID src, ULONG len)
{
    PVOID p = fort_mem_alloc(len, FORT_DEVICE_CONF_POOL_TAG);
    if (p != NULL) {
        RtlCopyMemory(p, src, len);
    }
    return p;
}

static void fort_conf_mem_free(PVOID p)
{
    if (p != NULL) {
        fort_mem_free(p, FORT_DEVICE_CONF_POOL_TAG);
    }
}

FORT_API void fort_device_conf_open(PFORT_DEVICE_CONF device_conf)
{
    KeInitializeSpinLock(&device_conf->ref_lock);
}

FORT_API UCHAR fort_device_flag_set(PFORT_DEVICE_CONF device_conf, UCHAR flag, BOOL on)
{
    return on ? InterlockedOr8(&device_conf->flags, flag)
              : InterlockedAnd8(&device_conf->flags, ~flag);
}

static UCHAR fort_device_flags(PFORT_DEVICE_CONF device_conf)
{
    return fort_device_flag_set(device_conf, 0, TRUE);
}

FORT_API UCHAR fort_device_flag(PFORT_DEVICE_CONF device_conf, UCHAR flag)
{
    return fort_device_flags(device_conf) & flag;
}

static PFORT_CONF_EXE_NODE fort_conf_ref_exe_find_node(
        PFORT_CONF_REF conf_ref, PCFORT_APP_PATH path, tommy_key_t path_hash)
{
    PFORT_CONF_EXE_NODE node =
            (PFORT_CONF_EXE_NODE) tommy_hashdyn_bucket(&conf_ref->exe_map, path_hash);

    while (node != NULL) {
        if (fort_conf_app_exe_equal(node->app_entry, path))
            return node;

        node = node->next;
    }

    return NULL;
}

FORT_API FORT_APP_DATA fort_conf_exe_find(
        const PFORT_CONF conf, PVOID context, PCFORT_APP_PATH path)
{
    UNUSED(conf);

    PFORT_CONF_REF conf_ref = context;
    const tommy_key_t path_hash = (tommy_key_t) tommy_hash_u64(0, path->buffer, path->len);

    FORT_APP_DATA app_data = { 0 };

    KIRQL oldIrql = ExAcquireSpinLockShared(&conf_ref->conf_lock);
    {
        const PFORT_CONF_EXE_NODE node = fort_conf_ref_exe_find_node(conf_ref, path, path_hash);

        if (node != NULL) {
            app_data = node->app_entry->app_data;
        }
    }
    ExReleaseSpinLockShared(&conf_ref->conf_lock, oldIrql);

    return app_data;
}

static void fort_conf_ref_exe_new_path(
        PFORT_CONF_REF conf_ref, PFORT_APP_ENTRY entry, tommy_key_t path_hash)
{
    PFORT_CONF conf = &conf_ref->conf;

    tommy_arrayof *exe_nodes = &conf_ref->exe_nodes;
    tommy_hashdyn *exe_map = &conf_ref->exe_map;

    tommy_hashdyn_node *exe_node = tommy_list_tail(&conf_ref->free_nodes);

    if (exe_node != NULL) {
        tommy_list_remove_existing(&conf_ref->free_nodes, exe_node);
    } else {
        const UINT16 index = conf->exe_apps_n;

        tommy_arrayof_grow(exe_nodes, index + 1);

        exe_node = tommy_arrayof_ref(exe_nodes, index);
    }

    tommy_hashdyn_insert(exe_map, exe_node, entry, path_hash);

    ++conf->exe_apps_n;
}

static NTSTATUS fort_conf_ref_exe_new_entry(PFORT_CONF_REF conf_ref, PCFORT_APP_ENTRY app_entry,
        PCFORT_APP_PATH path, tommy_key_t path_hash)
{
    const UINT16 path_len = path->len;

    const UINT16 entry_size = (UINT16) FORT_CONF_APP_ENTRY_SIZE(path_len);
    PFORT_APP_ENTRY entry = fort_pool_malloc(&conf_ref->pool_list, entry_size);

    if (entry == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    entry->app_data = app_entry->app_data;
    entry->path_len = path_len;

    /* Copy the path */
    {
        RtlCopyMemory(entry->path, path->buffer, path_len);
        entry->path[path_len / sizeof(WCHAR)] = L'\0';
    }

    /* Add exe node */
    fort_conf_ref_exe_new_path(conf_ref, entry, path_hash);

    return STATUS_SUCCESS;
}

static NTSTATUS fort_conf_ref_exe_add_path_locked(PFORT_CONF_REF conf_ref,
        PCFORT_APP_ENTRY app_entry, PCFORT_APP_PATH path, tommy_key_t path_hash)
{
    const PFORT_CONF_EXE_NODE node = fort_conf_ref_exe_find_node(conf_ref, path, path_hash);

    if (node == NULL) {
        return fort_conf_ref_exe_new_entry(conf_ref, app_entry, path, path_hash);
    }

    if (app_entry->app_data.is_new)
        return FORT_STATUS_USER_ERROR;

    /* Replace the app data */
    {
        PFORT_APP_ENTRY entry = node->app_entry;
        entry->app_data = app_entry->app_data;
    }

    return STATUS_SUCCESS;
}

FORT_API NTSTATUS fort_conf_ref_exe_add_path(
        PFORT_CONF_REF conf_ref, PCFORT_APP_ENTRY app_entry, PCFORT_APP_PATH path)
{
    const tommy_key_t path_hash = (tommy_key_t) tommy_hash_u64(0, path->buffer, path->len);
    NTSTATUS status;

    KIRQL oldIrql = ExAcquireSpinLockExclusive(&conf_ref->conf_lock);
    status = fort_conf_ref_exe_add_path_locked(conf_ref, app_entry, path, path_hash);
    ExReleaseSpinLockExclusive(&conf_ref->conf_lock, oldIrql);

    return status;
}

FORT_API NTSTATUS fort_conf_ref_exe_add_entry(
        PFORT_CONF_REF conf_ref, PCFORT_APP_ENTRY app_entry, BOOL locked)
{
    const FORT_APP_PATH path = {
        .len = app_entry->path_len,
        .buffer = app_entry->path,
    };

    if (locked) {
        const tommy_key_t path_hash = (tommy_key_t) tommy_hash_u64(0, path.buffer, path.len);

        return fort_conf_ref_exe_add_path_locked(conf_ref, app_entry, &path, path_hash);
    } else {
        return fort_conf_ref_exe_add_path(conf_ref, app_entry, &path);
    }
}

static void fort_conf_ref_exe_fill(PFORT_CONF_REF conf_ref, const PFORT_CONF conf)
{
    const char *app_entries = (const char *) (conf->data + conf->exe_apps_off);

    const int count = conf->exe_apps_n;

    for (int i = 0; i < count; ++i) {
        PCFORT_APP_ENTRY entry = (PCFORT_APP_ENTRY) app_entries;

        fort_conf_ref_exe_add_entry(conf_ref, entry, TRUE);

        app_entries += FORT_CONF_APP_ENTRY_SIZE(entry->path_len);
    }
}

static void fort_conf_ref_exe_del_path(PFORT_CONF_REF conf_ref, PCFORT_APP_PATH path)
{
    const tommy_key_t path_hash = (tommy_key_t) tommy_hash_u64(0, path->buffer, path->len);

    KIRQL oldIrql = ExAcquireSpinLockExclusive(&conf_ref->conf_lock);
    {
        PFORT_CONF_EXE_NODE node = fort_conf_ref_exe_find_node(conf_ref, path, path_hash);

        if (node != NULL) {
            /* Delete from conf */
            {
                PFORT_CONF conf = &conf_ref->conf;
                --conf->exe_apps_n;
            }

            /* Delete from pool */
            {
                PFORT_APP_ENTRY entry = node->app_entry;
                fort_pool_free(&conf_ref->pool_list, entry);
            }

            /* Delete from exe map */
            tommy_hashdyn_remove_existing(&conf_ref->exe_map, (tommy_hashdyn_node *) node);

            tommy_list_insert_tail_check(&conf_ref->free_nodes, (tommy_node *) node);
        }
    }
    ExReleaseSpinLockExclusive(&conf_ref->conf_lock, oldIrql);
}

FORT_API void fort_conf_ref_exe_del_entry(PFORT_CONF_REF conf_ref, PCFORT_APP_ENTRY entry)
{
    const FORT_APP_PATH path = {
        .len = entry->path_len,
        .buffer = entry->path,
    };

    fort_conf_ref_exe_del_path(conf_ref, &path);
}

static void fort_conf_ref_init(PFORT_CONF_REF conf_ref)
{
    conf_ref->refcount = 0;

    fort_pool_list_init(&conf_ref->pool_list);
    tommy_list_init(&conf_ref->free_nodes);

    tommy_arrayof_init(&conf_ref->exe_nodes, sizeof(FORT_CONF_EXE_NODE));
    tommy_hashdyn_init(&conf_ref->exe_map);

    conf_ref->conf_lock = 0;
}

FORT_API PFORT_CONF_REF fort_conf_ref_new(const PFORT_CONF conf, ULONG len)
{
    const ULONG conf_len = FORT_CONF_DATA_OFF + conf->exe_apps_off;
    const ULONG ref_len = conf_len + offsetof(FORT_CONF_REF, conf);
    PFORT_CONF_REF conf_ref = tommy_malloc(ref_len);

    if (conf_ref != NULL) {
        RtlCopyMemory(&conf_ref->conf, conf, conf_len);

        fort_conf_ref_init(conf_ref);
        fort_pool_init(&conf_ref->pool_list, len - conf_len);

        fort_conf_ref_exe_fill(conf_ref, conf);
    }

    return conf_ref;
}

static void fort_conf_ref_del(PFORT_CONF_REF conf_ref)
{
    fort_pool_done(&conf_ref->pool_list);

    tommy_hashdyn_done(&conf_ref->exe_map);
    tommy_arrayof_done(&conf_ref->exe_nodes);

    tommy_free(conf_ref);
}

static void fort_conf_ref_put_locked(PFORT_DEVICE_CONF device_conf, PFORT_CONF_REF conf_ref)
{
    const UINT32 refcount = --conf_ref->refcount;

    if (refcount == 0 && conf_ref != device_conf->ref) {
        fort_conf_ref_del(conf_ref);
    }
}

FORT_API void fort_conf_ref_put(PFORT_DEVICE_CONF device_conf, PFORT_CONF_REF conf_ref)
{
    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&device_conf->ref_lock, &lock_queue);
    {
        fort_conf_ref_put_locked(device_conf, conf_ref);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

FORT_API PFORT_CONF_REF fort_conf_ref_take(PFORT_DEVICE_CONF device_conf)
{
    if (device_conf->ref == NULL)
        return NULL;

    PFORT_CONF_REF conf_ref;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&device_conf->ref_lock, &lock_queue);
    {
        conf_ref = device_conf->ref;
        if (conf_ref != NULL) {
            ++conf_ref->refcount;
        }
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return conf_ref;
}

FORT_API FORT_CONF_FLAGS fort_conf_ref_set(PFORT_DEVICE_CONF device_conf, PFORT_CONF_REF conf_ref)
{
    FORT_CONF_FLAGS old_conf_flags;

    const PFORT_CONF_REF old_conf_ref = fort_conf_ref_take(device_conf);

    if (old_conf_ref != NULL) {
        old_conf_flags = old_conf_ref->conf.flags;
    } else {
        const UCHAR flags = fort_device_flag(device_conf, FORT_DEVICE_BOOT_MASK);

        RtlZeroMemory(&old_conf_flags, sizeof(FORT_CONF_FLAGS));
        old_conf_flags.boot_filter = (flags & FORT_DEVICE_BOOT_FILTER) != 0;
        old_conf_flags.filter_locals = (flags & FORT_DEVICE_BOOT_FILTER_LOCALS) != 0;
    }

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&device_conf->ref_lock, &lock_queue);
    {
        FORT_CONF_FLAGS conf_flags;

        device_conf->ref = conf_ref;

        if (conf_ref != NULL) {
            PFORT_CONF conf = &conf_ref->conf;

            conf_flags = conf->flags;
            fort_device_flag_set(device_conf, FORT_DEVICE_BOOT_FILTER, conf_flags.boot_filter);
            fort_device_flag_set(
                    device_conf, FORT_DEVICE_BOOT_FILTER_LOCALS, conf_flags.filter_locals);
        } else {
            RtlZeroMemory((void *) &conf_flags, sizeof(FORT_CONF_FLAGS));
            conf_flags.boot_filter = old_conf_flags.boot_filter;
            conf_flags.filter_locals = old_conf_flags.filter_locals;
        }

        device_conf->conf_flags = conf_flags;

        if (old_conf_ref != NULL) {
            fort_conf_ref_put_locked(device_conf, old_conf_ref);
        }
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return old_conf_flags;
}

FORT_API FORT_CONF_FLAGS fort_conf_ref_flags_set(
        PFORT_DEVICE_CONF device_conf, const FORT_CONF_FLAGS conf_flags)
{
    FORT_CONF_FLAGS old_conf_flags;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&device_conf->ref_lock, &lock_queue);
    {
        PFORT_CONF_REF conf_ref = device_conf->ref;

        if (conf_ref != NULL) {
            PFORT_CONF conf = &conf_ref->conf;

            old_conf_flags = conf->flags;
            conf->flags = conf_flags;

            fort_device_flag_set(device_conf, FORT_DEVICE_BOOT_FILTER, conf_flags.boot_filter);
            fort_device_flag_set(
                    device_conf, FORT_DEVICE_BOOT_FILTER_LOCALS, conf_flags.filter_locals);

            device_conf->conf_flags = conf_flags;
        } else {
            const UCHAR flags = fort_device_flag(device_conf, FORT_DEVICE_BOOT_MASK);

            RtlZeroMemory(&old_conf_flags, sizeof(FORT_CONF_FLAGS));
            old_conf_flags.boot_filter = (flags & FORT_DEVICE_BOOT_FILTER) != 0;
            old_conf_flags.filter_locals = (flags & FORT_DEVICE_BOOT_FILTER_LOCALS) != 0;

            device_conf->conf_flags = old_conf_flags;
        }
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return old_conf_flags;
}

FORT_API PFORT_SERVICE_SID_LIST fort_conf_service_sids_new(
        PFORT_SERVICE_SID_LIST service_sids, ULONG len)
{
    return fort_conf_mem_alloc(service_sids, len);
}

FORT_API void fort_conf_service_sids_set(
        PFORT_DEVICE_CONF device_conf, PFORT_SERVICE_SID_LIST service_sids)
{
    KIRQL oldIrql = ExAcquireSpinLockExclusive(&device_conf->lock);
    {
        fort_conf_mem_free(device_conf->service_sids);
        device_conf->service_sids = service_sids;
    }
    ExReleaseSpinLockExclusive(&device_conf->lock, oldIrql);
}

FORT_API BOOL fort_conf_get_service_sid_path(
        PFORT_DEVICE_CONF device_conf, const char *sidBytes, PFORT_APP_PATH path)
{
    char *buffer = (char *) path->buffer;

    path->len = 0;

    KIRQL oldIrql = ExAcquireSpinLockExclusive(&device_conf->lock);
    {
        PCWSTR service_name = fort_conf_service_sid_name_find(device_conf->service_sids, sidBytes);
        if (service_name != NULL) {
            char *name_buf = buffer + FORT_SVCHOST_PREFIX_SIZE;
            const DWORD name_size = (DWORD) (wcslen(service_name) * sizeof(WCHAR));

            RtlCopyMemory(
                    name_buf, service_name, name_size + sizeof(WCHAR)); /* + null terminator */

            path->len = (UINT16) (FORT_SVCHOST_PREFIX_SIZE + name_size);
        }
    }
    ExReleaseSpinLockExclusive(&device_conf->lock, oldIrql);

    if (path->len == 0)
        return FALSE;

    RtlCopyMemory(buffer, FORT_SVCHOST_PREFIX, FORT_SVCHOST_PREFIX_SIZE);

    return TRUE;
}

FORT_API PFORT_CONF_ZONES fort_conf_zones_new(PFORT_CONF_ZONES zones, ULONG len)
{
    return fort_conf_mem_alloc(zones, len);
}

FORT_API void fort_conf_zones_set(PFORT_DEVICE_CONF device_conf, PFORT_CONF_ZONES zones)
{
    KIRQL oldIrql = ExAcquireSpinLockExclusive(&device_conf->lock);
    {
        fort_conf_mem_free(device_conf->zones);
        device_conf->zones = zones;
    }
    ExReleaseSpinLockExclusive(&device_conf->lock, oldIrql);
}

FORT_API void fort_conf_zone_flag_set(PFORT_DEVICE_CONF device_conf, PFORT_CONF_ZONE_FLAG zone_flag)
{
    KIRQL oldIrql = ExAcquireSpinLockExclusive(&device_conf->lock);
    PFORT_CONF_ZONES zones = device_conf->zones;
    if (zones != NULL) {
        const UINT32 zone_mask = (1u << (zone_flag->zone_id - 1));
        if (zone_flag->enabled) {
            zones->enabled_mask |= zone_mask;
        } else {
            zones->enabled_mask &= ~zone_mask;
        }
    }
    ExReleaseSpinLockExclusive(&device_conf->lock, oldIrql);
}

FORT_API BOOL fort_conf_zones_ip_included(
        PFORT_DEVICE_CONF device_conf, UINT32 zones_mask, const UINT32 *remote_ip, BOOL isIPv6)
{
    BOOL res = FALSE;

    KIRQL oldIrql = ExAcquireSpinLockShared(&device_conf->lock);
    PFORT_CONF_ZONES zones = device_conf->zones;
    if (zones != NULL) {
        zones_mask &= (zones->mask & zones->enabled_mask);
        while (zones_mask != 0) {
            const int zone_index = bit_scan_forward(zones_mask);
            PFORT_CONF_ADDR_LIST addr_list =
                    (PFORT_CONF_ADDR_LIST) (zones->data + zones->addr_off[zone_index]);

            if (fort_conf_ip_inlist(remote_ip, addr_list, isIPv6)) {
                res = TRUE;
                break;
            }

            zones_mask ^= (1u << zone_index);
        }
    }
    ExReleaseSpinLockShared(&device_conf->lock, oldIrql);

    return res;
}
