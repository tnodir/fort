/* Fort Firewall Configuration */

#include "fortcnf.h"
#include "forttrace.h"

#define FORT_DEVICE_CONF_POOL_TAG 'CwfF'

/* Synchronize with tommy_hashdyn_node! */
typedef struct fort_conf_exe_node
{
    struct fort_conf_exe_node *next;
    struct fort_conf_exe_node *prev;

    PFORT_APP_ENTRY app_entry; /* tommy_hashdyn_node::data */

    tommy_key_t path_hash; /* tommy_hashdyn_node::index */
} FORT_CONF_EXE_NODE, *PFORT_CONF_EXE_NODE;

typedef const FORT_CONF_EXE_NODE *PCFORT_CONF_EXE_NODE;

static PVOID fort_conf_mem_alloc(const void *src, ULONG len)
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

FORT_API UINT16 fort_device_flag_set(PFORT_DEVICE_CONF device_conf, UINT16 flag, BOOL on)
{
    return on ? InterlockedOr16(&device_conf->flags, flag)
              : InterlockedAnd16(&device_conf->flags, ~flag);
}

static UINT16 fort_device_flags(PFORT_DEVICE_CONF device_conf)
{
    return fort_device_flag_set(device_conf, 0, TRUE);
}

FORT_API UINT16 fort_device_flag(PFORT_DEVICE_CONF device_conf, UINT16 flag)
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

FORT_API FORT_APP_DATA fort_conf_exe_find(PCFORT_CONF conf, PVOID context, PCFORT_APP_PATH path)
{
    UNUSED(conf);

    PFORT_CONF_REF conf_ref = context;
    const tommy_key_t path_hash = (tommy_key_t) tommy_hash_u64(0, path->buffer, path->len);

    FORT_APP_DATA app_data = { 0 };

    KIRQL oldIrql = ExAcquireSpinLockShared(&conf_ref->conf_lock);
    {
        PCFORT_CONF_EXE_NODE node = fort_conf_ref_exe_find_node(conf_ref, path, path_hash);

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
    PCFORT_CONF_EXE_NODE node = fort_conf_ref_exe_find_node(conf_ref, path, path_hash);

    if (node == NULL) {
        return fort_conf_ref_exe_new_entry(conf_ref, app_entry, path, path_hash);
    }

    if (app_entry->app_data.flags.is_new)
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
    {
        status = fort_conf_ref_exe_add_path_locked(conf_ref, app_entry, path, path_hash);
    }
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

static void fort_conf_ref_exe_fill(PFORT_CONF_REF conf_ref, PCFORT_CONF conf)
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

FORT_API PFORT_CONF_REF fort_conf_ref_new(PCFORT_CONF conf, ULONG len)
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

static void fort_device_flags_conf_set(PFORT_DEVICE_CONF device_conf, FORT_CONF_FLAGS conf_flags)
{
    fort_device_flag_set(device_conf, FORT_DEVICE_BOOT_FILTER, conf_flags.boot_filter);
    fort_device_flag_set(device_conf, FORT_DEVICE_STEALTH_MODE, conf_flags.stealth_mode);
    fort_device_flag_set(device_conf, FORT_DEVICE_FILTER_LOCALS, conf_flags.filter_locals);
    fort_device_flag_set(device_conf, FORT_DEVICE_TRACE_EVENTS, conf_flags.trace_events);
}

static void fort_device_flags_conf_copy(FORT_CONF_FLAGS *conf_flags, const UINT16 flags)
{
    RtlZeroMemory(conf_flags, sizeof(FORT_CONF_FLAGS));
    conf_flags->boot_filter = (flags & FORT_DEVICE_BOOT_FILTER) != 0;
    conf_flags->stealth_mode = (flags & FORT_DEVICE_STEALTH_MODE) != 0;
    conf_flags->filter_locals = (flags & FORT_DEVICE_FILTER_LOCALS) != 0;
    conf_flags->trace_events = (flags & FORT_DEVICE_TRACE_EVENTS) != 0;
}

static void fort_device_flags_conf_log_event(
        const FORT_CONF_FLAGS old_conf_flags, const FORT_CONF_FLAGS conf_flags)
{
    if (!conf_flags.trace_events)
        return;

    if (old_conf_flags.filter_enabled != conf_flags.filter_enabled) {
        TRACE(FORT_CONFIG_FILTER_OFF + conf_flags.filter_enabled, 0, 0, 0);
    }

    if (old_conf_flags.filter_mode != conf_flags.filter_mode) {
        TRACE(FORT_CONFIG_FILTER_MODE_AUTO_LEARN + conf_flags.filter_mode, 0, 0, 0);
    }
}

FORT_API FORT_CONF_FLAGS fort_conf_ref_set(PFORT_DEVICE_CONF device_conf, PFORT_CONF_REF conf_ref)
{
    FORT_CONF_FLAGS old_conf_flags;
    FORT_CONF_FLAGS conf_flags;

    const PFORT_CONF_REF old_conf_ref = fort_conf_ref_take(device_conf);

    if (old_conf_ref != NULL) {
        old_conf_flags = old_conf_ref->conf.flags;
    } else {
        const UINT16 flags = fort_device_flags(device_conf);

        fort_device_flags_conf_copy(&old_conf_flags, flags);
    }

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&device_conf->ref_lock, &lock_queue);
    {
        device_conf->ref = conf_ref;

        if (conf_ref != NULL) {
            PFORT_CONF conf = &conf_ref->conf;

            conf_flags = conf->flags;

            fort_device_flags_conf_set(device_conf, conf_flags);
        } else {
            RtlZeroMemory((void *) &conf_flags, sizeof(FORT_CONF_FLAGS));
            conf_flags.boot_filter = old_conf_flags.boot_filter;
            conf_flags.stealth_mode = old_conf_flags.stealth_mode;
            conf_flags.filter_locals = old_conf_flags.filter_locals;
            conf_flags.trace_events = old_conf_flags.trace_events;
        }

        device_conf->conf_flags = conf_flags;

        if (old_conf_ref != NULL) {
            fort_conf_ref_put_locked(device_conf, old_conf_ref);
        }
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    fort_device_flags_conf_log_event(old_conf_flags, conf_flags);

    return old_conf_flags;
}

FORT_API FORT_CONF_FLAGS fort_conf_ref_flags_set(
        PFORT_DEVICE_CONF device_conf, FORT_CONF_FLAGS conf_flags)
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

            fort_device_flags_conf_set(device_conf, conf_flags);
        } else {
            const UINT16 flags = fort_device_flags(device_conf);

            fort_device_flags_conf_copy(&old_conf_flags, flags);

            conf_flags = old_conf_flags;
        }

        device_conf->conf_flags = conf_flags;
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    fort_device_flags_conf_log_event(old_conf_flags, conf_flags);

    return old_conf_flags;
}

FORT_API PFORT_CONF_ZONES fort_conf_zones_new(PCFORT_CONF_ZONES zones, ULONG len)
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

inline static void fort_conf_zone_flag_set_locked(
        PFORT_CONF_ZONES zones, PCFORT_CONF_ZONE_FLAG zone_flag)
{
    const UINT32 zone_mask = (1u << (zone_flag->zone_id - 1));

    if (zone_flag->enabled) {
        zones->enabled_mask |= zone_mask;
    } else {
        zones->enabled_mask &= ~zone_mask;
    }
}

FORT_API void fort_conf_zone_flag_set(
        PFORT_DEVICE_CONF device_conf, PCFORT_CONF_ZONE_FLAG zone_flag)
{
    KIRQL oldIrql = ExAcquireSpinLockExclusive(&device_conf->lock);
    PFORT_CONF_ZONES zones = device_conf->zones;
    if (zones != NULL) {
        fort_conf_zone_flag_set_locked(zones, zone_flag);
    }
    ExReleaseSpinLockExclusive(&device_conf->lock, oldIrql);
}

FORT_API BOOL fort_devconf_zones_ip_included(PFORT_DEVICE_CONF device_conf,
        PCFORT_CONF_META_CONN conn, UCHAR *zone_id, UINT32 zones_mask)
{
    BOOL res = FALSE;

    KIRQL oldIrql = ExAcquireSpinLockShared(&device_conf->lock);
    PCFORT_CONF_ZONES zones = device_conf->zones;
    if (zones != NULL) {
        res = fort_conf_zones_ip_included(zones, conn, zone_id, zones_mask);
    }
    ExReleaseSpinLockShared(&device_conf->lock, oldIrql);

    return res;
}

FORT_API BOOL fort_devconf_zones_conn_filtered(PFORT_DEVICE_CONF device_conf,
        PCFORT_CONF_META_CONN conn, PFORT_CONF_ZONES_CONN_FILTERED_OPT opt)
{
    BOOL res = FALSE;

    KIRQL oldIrql = ExAcquireSpinLockShared(&device_conf->lock);
    PCFORT_CONF_ZONES zones = device_conf->zones;
    if (zones != NULL) {
        res = fort_conf_zones_conn_filtered(zones, conn, opt);
    }
    ExReleaseSpinLockShared(&device_conf->lock, oldIrql);

    return res;
}

FORT_API PFORT_CONF_RULES fort_conf_rules_new(PCFORT_CONF_RULES rules, ULONG len)
{
    return fort_conf_mem_alloc(rules, len);
}

FORT_API void fort_conf_rules_set(PFORT_DEVICE_CONF device_conf, PFORT_CONF_RULES rules)
{
    KIRQL oldIrql = ExAcquireSpinLockExclusive(&device_conf->lock);
    {
        fort_conf_mem_free(device_conf->rules);
        device_conf->rules = rules;

        if (rules != NULL) {
            device_conf->rules_glob = rules->glob;
        } else {
            const FORT_CONF_RULES_GLOB rules_glob = { 0 };
            device_conf->rules_glob = rules_glob;
        }
    }
    ExReleaseSpinLockExclusive(&device_conf->lock, oldIrql);
}

inline static void fort_conf_rule_flag_set_locked(
        PFORT_CONF_RULES rules, PCFORT_CONF_RULE_FLAG rule_flag)
{
    if (rule_flag->rule_id > rules->max_rule_id)
        return;

    const FORT_CONF_RULES_RT rules_rt = fort_conf_rules_rt_make(rules, /*zones=*/NULL);
    PFORT_CONF_RULE rule = fort_conf_rules_rt_rule(&rules_rt, rule_flag->rule_id);

    rule->enabled = rule_flag->enabled;
}

FORT_API void fort_conf_rule_flag_set(
        PFORT_DEVICE_CONF device_conf, PCFORT_CONF_RULE_FLAG rule_flag)
{
    KIRQL oldIrql = ExAcquireSpinLockExclusive(&device_conf->lock);
    PFORT_CONF_RULES rules = device_conf->rules;
    if (rules != NULL) {
        fort_conf_rule_flag_set_locked(rules, rule_flag);
    }
    ExReleaseSpinLockExclusive(&device_conf->lock, oldIrql);
}

FORT_API BOOL fort_devconf_rules_conn_filtered(
        PFORT_DEVICE_CONF device_conf, PFORT_CONF_META_CONN conn, UINT16 rule_id)
{
    BOOL res = FALSE;

    KIRQL oldIrql = ExAcquireSpinLockExclusive(&device_conf->lock);
    PFORT_CONF_RULES rules = device_conf->rules;
    if (rules != NULL) {
        res = fort_conf_rules_conn_filtered(rules, device_conf->zones, conn, rule_id);
    }
    ExReleaseSpinLockExclusive(&device_conf->lock, oldIrql);

    return res;
}
