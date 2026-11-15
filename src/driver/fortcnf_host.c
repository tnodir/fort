/* Fort Firewall Configuration: Hosts' IPs */

#include "fortcnf_host.h"

#include "common/fortcmnutl.h"

#define FORT_HOST_IPS_POOL_SIZE (16 * 1024)

#define FORT_HOST_IPS_ZONES_ALL 0xFFFFFFFF

#define FORT_INTERRUPT_TIME_SECOND 10000000LL

/* The established connections outlive the TTL: their reauthorization keeps the expired IP */
#define FORT_HOST_IP_REAUTH_TIME (24 * 60 * 60 * FORT_INTERRUPT_TIME_SECOND)

/* The full table's purge walks all IPs under the lock: not on each new IP */
#define FORT_HOST_IPS_PURGE_PERIOD (60 * FORT_INTERRUPT_TIME_SECOND)

typedef struct fort_host_ips_add_arg
{
    INT64 now;
    INT64 expire_time;

    PCFORT_CONF_HOST_IPS conf_ips;
} FORT_HOST_IPS_ADD_ARG, *PFORT_HOST_IPS_ADD_ARG;

typedef const FORT_HOST_IPS_ADD_ARG *PCFORT_HOST_IPS_ADD_ARG;

typedef struct fort_host_ips_purge_arg
{
    BOOL keep_reauth; /* keep the IPs expired by the TTL, but not for the reauthorization */

    UINT32 zones_mask;

    INT64 now;

    PFORT_HOST_IP purged_ips; /* chained by purge_next */
} FORT_HOST_IPS_PURGE_ARG, *PFORT_HOST_IPS_PURGE_ARG;

typedef const FORT_HOST_IPS_PURGE_ARG *PCFORT_HOST_IPS_PURGE_ARG;

typedef struct fort_host_ips_filter_arg
{
    BOOL is_reauth;

    tommy_hash_t hash;

    INT64 now;

    PCFORT_CONF_RULE_FILTER_HOST host;

    FORT_HOST_IP key; /* the connection's remote IP */
} FORT_HOST_IPS_FILTER_ARG, *PFORT_HOST_IPS_FILTER_ARG;

typedef const FORT_HOST_IPS_FILTER_ARG *PCFORT_HOST_IPS_FILTER_ARG;

FORT_API void fort_host_ips_open(PFORT_HOST_IPS host_ips)
{
    fort_pool_list_init(&host_ips->pool_list);
    fort_pool_init(&host_ips->pool_list, FORT_HOST_IPS_POOL_SIZE);

    tommy_hashdyn_init(&host_ips->ips_map);
}

FORT_API void fort_host_ips_close(PFORT_HOST_IPS host_ips)
{
    KIRQL oldIrql = ExAcquireSpinLockExclusive(&host_ips->lock);
    {
        host_ips->count = 0;

        fort_pool_done(&host_ips->pool_list);

        tommy_hashdyn_done(&host_ips->ips_map);
    }
    ExReleaseSpinLockExclusive(&host_ips->lock, oldIrql);
}

/* Returns the host() filter's name's id, 0 - the Zones' IP */
inline static UINT64 fort_host_ip_filter_id(PCFORT_HOST_IP host_ip)
{
    return host_ip->isFilter ? host_ip->name_id : 0;
}

inline static FORT_HOST_IP fort_host_ip_conn_key(PCFORT_CONF_META_CONN conn)
{
    const FORT_HOST_IP key = {
        .isIPv6 = (UCHAR) conn->isIPv6,
        .ip = conn->remote_ip,
    };

    return key;
}

/* By the IP only: a host() filter's check walks the IP's names in its bucket */
inline static tommy_hash_t fort_host_ip_hash(PCFORT_HOST_IP key)
{
    return tommy_hash_u32(0, &key->ip, key->isIPv6 ? sizeof(ip6_addr_t) : sizeof(UINT32));
}

static int fort_host_ip_addr_cmp(PCFORT_HOST_IP key, PCFORT_HOST_IP host_ip)
{
    if (key->isIPv6 != host_ip->isIPv6)
        return 1;

    return key->isIPv6 ? fort_ip6_cmp(&key->ip.v6, &host_ip->ip.v6)
                       : (key->ip.v4 != host_ip->ip.v4);
}

static int fort_host_ip_cmp(const void *arg, const void *obj)
{
    PCFORT_HOST_IP key = arg;
    PCFORT_HOST_IP host_ip = obj;

    if (fort_host_ip_filter_id(key) != fort_host_ip_filter_id(host_ip))
        return 1;

    return fort_host_ip_addr_cmp(key, host_ip);
}

static void fort_host_ip_del_locked(PFORT_HOST_IPS host_ips, PFORT_HOST_IP host_ip)
{
    tommy_hashdyn_remove_existing(&host_ips->ips_map, &host_ip->map_node);

    fort_pool_free(&host_ips->pool_list, host_ip);

    --host_ips->count;
}

inline static BOOL fort_host_ip_expired(PCFORT_HOST_IP host_ip, BOOL is_reauth, INT64 now)
{
    const INT64 reauth_time = is_reauth ? FORT_HOST_IP_REAUTH_TIME : 0;

    return host_ip->expire_time + reauth_time <= now;
}

static BOOL fort_host_ip_purged(PFORT_HOST_IP host_ip, PCFORT_HOST_IPS_PURGE_ARG arg)
{
    if (fort_host_ip_expired(host_ip, arg->keep_reauth, arg->now))
        return TRUE;

    if (host_ip->isFilter)
        return FALSE;

    host_ip->zones_mask &= arg->zones_mask;

    return host_ip->zones_mask == 0;
}

static void fort_host_ip_purge_check(void *arg, void *obj)
{
    PFORT_HOST_IPS_PURGE_ARG purge_arg = arg;
    PFORT_HOST_IP host_ip = obj;

    if (!fort_host_ip_purged(host_ip, purge_arg))
        return;

    host_ip->purge_next = purge_arg->purged_ips;
    purge_arg->purged_ips = host_ip;
}

/* Removes the expired IPs (for the reauthorization too, if kept) and the Zones not in the mask */
static void fort_host_ips_purge_locked(PFORT_HOST_IPS host_ips, PFORT_HOST_IPS_PURGE_ARG arg)
{
    arg->purged_ips = NULL;

    /* A removal may shrink the map's buckets: the walk only collects the IPs */
    tommy_hashdyn_foreach_arg(&host_ips->ips_map, &fort_host_ip_purge_check, arg);

    PFORT_HOST_IP host_ip = arg->purged_ips;

    while (host_ip != NULL) {
        PFORT_HOST_IP next_ip = host_ip->purge_next;

        fort_host_ip_del_locked(host_ips, host_ip);

        host_ip = next_ip;
    }
}

inline static BOOL fort_host_ips_full_locked(PFORT_HOST_IPS host_ips, INT64 now)
{
    if (host_ips->count < FORT_CONF_HOST_IPS_MAX)
        return FALSE;

    if (now < host_ips->purge_time)
        return TRUE;

    host_ips->purge_time = now + FORT_HOST_IPS_PURGE_PERIOD;

    FORT_HOST_IPS_PURGE_ARG arg = {
        .keep_reauth = TRUE,
        .zones_mask = FORT_HOST_IPS_ZONES_ALL,
        .now = now,
    };

    fort_host_ips_purge_locked(host_ips, &arg);

    if (host_ips->count < FORT_CONF_HOST_IPS_MAX)
        return FALSE;

    /* The established connections to the IPs expired by the TTL lose their Zones and host()
     * filters on the reauthorization */
    arg.keep_reauth = FALSE;

    fort_host_ips_purge_locked(host_ips, &arg);

    return host_ips->count >= FORT_CONF_HOST_IPS_MAX;
}

static PFORT_HOST_IP fort_host_ip_new_locked(
        PFORT_HOST_IPS host_ips, PCFORT_HOST_IP key, tommy_hash_t hash, INT64 now)
{
    if (fort_host_ips_full_locked(host_ips, now))
        return NULL;

    PFORT_HOST_IP host_ip = fort_pool_malloc(&host_ips->pool_list, sizeof(FORT_HOST_IP));
    if (host_ip == NULL)
        return NULL;

    *host_ip = *key;

    tommy_hashdyn_insert(&host_ips->ips_map, &host_ip->map_node, host_ip, hash);

    ++host_ips->count;

    return host_ip;
}

static PFORT_HOST_IP fort_host_ip_get_locked(PFORT_HOST_IPS host_ips, PCFORT_HOST_IP key, INT64 now)
{
    const tommy_hash_t hash = fort_host_ip_hash(key);

    PFORT_HOST_IP host_ip = tommy_hashdyn_search(&host_ips->ips_map, &fort_host_ip_cmp, key, hash);
    if (host_ip != NULL)
        return host_ip;

    return fort_host_ip_new_locked(host_ips, key, hash, now);
}

static void fort_host_ip_zones_add(PFORT_HOST_IP host_ip, UINT32 zones_mask, INT64 now)
{
    /* The IP expired for the reauthorization too loses its old Zones; else they stay for the
     * established connections and are matched by the new ones too */
    if (fort_host_ip_expired(host_ip, /*is_reauth=*/TRUE, now)) {
        host_ip->zones_mask = 0;
    }

    host_ip->zones_mask |= zones_mask;
}

static NTSTATUS fort_host_ip_add_locked(
        PFORT_HOST_IPS host_ips, PCFORT_HOST_IPS_ADD_ARG arg, PCFORT_HOST_IP key)
{
    PFORT_HOST_IP host_ip = fort_host_ip_get_locked(host_ips, key, arg->now);
    if (host_ip == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    if (!host_ip->isFilter) {
        fort_host_ip_zones_add(host_ip, arg->conf_ips->zones_mask, arg->now);
    }

    if (host_ip->expire_time < arg->expire_time) {
        host_ip->expire_time = arg->expire_time;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS fort_host_ip_add_zones_locked(
        PFORT_HOST_IPS host_ips, PCFORT_HOST_IPS_ADD_ARG arg, PCFORT_HOST_IP ip_key)
{
    if (arg->conf_ips->zones_mask == 0)
        return STATUS_SUCCESS;

    return fort_host_ip_add_locked(host_ips, arg, ip_key);
}

/* Adds the IP to the Zones and to the host() filters */
static NTSTATUS fort_host_ip_add_all_locked(
        PFORT_HOST_IPS host_ips, PCFORT_HOST_IPS_ADD_ARG arg, PCFORT_HOST_IP ip_key)
{
    PCFORT_CONF_HOST_IPS conf_ips = arg->conf_ips;
    const UINT64 *name_ids = (const UINT64 *) conf_ips->data;

    NTSTATUS status = fort_host_ip_add_zones_locked(host_ips, arg, ip_key);

    for (UINT32 i = 0; NT_SUCCESS(status) && i < conf_ips->name_n; ++i) {
        FORT_HOST_IP key = *ip_key;
        key.isFilter = TRUE;
        key.name_id = name_ids[i];

        status = fort_host_ip_add_locked(host_ips, arg, &key);
    }

    return status;
}

static NTSTATUS fort_host_ips_add_locked(PFORT_HOST_IPS host_ips, PCFORT_HOST_IPS_ADD_ARG arg)
{
    PCFORT_CONF_HOST_IPS conf_ips = arg->conf_ips;

    const UINT32 ip4_n = conf_ips->ip4_n;
    const UINT32 ip_n = ip4_n + conf_ips->ip6_n;

    const char *data = conf_ips->data + FORT_CONF_HOST_NAME_IDS_SIZE(conf_ips->name_n);

    for (UINT32 i = 0; i < ip_n; ++i) {
        const BOOL isIPv6 = (i >= ip4_n);
        const UINT32 ip_size = isIPv6 ? sizeof(ip6_addr_t) : sizeof(UINT32);

        FORT_HOST_IP key = { .isIPv6 = (UCHAR) isIPv6 };
        RtlCopyMemory(key.ip.data, data, ip_size);
        data += ip_size;

        const NTSTATUS status = fort_host_ip_add_all_locked(host_ips, arg, &key);
        if (!NT_SUCCESS(status))
            return status;
    }

    return STATUS_SUCCESS;
}

FORT_API NTSTATUS fort_host_ips_add(PFORT_HOST_IPS host_ips, PCFORT_CONF_HOST_IPS conf_ips)
{
    const INT64 now = (INT64) KeQueryInterruptTime();

    const FORT_HOST_IPS_ADD_ARG arg = {
        .now = now,
        .expire_time = now + (INT64) conf_ips->ttl * FORT_INTERRUPT_TIME_SECOND,
        .conf_ips = conf_ips,
    };

    NTSTATUS status;

    KIRQL oldIrql = ExAcquireSpinLockExclusive(&host_ips->lock);
    {
        status = fort_host_ips_add_locked(host_ips, &arg);
    }
    ExReleaseSpinLockExclusive(&host_ips->lock, oldIrql);

    return status;
}

FORT_API void fort_host_ips_zones_keep(PFORT_HOST_IPS host_ips, UINT32 zones_mask)
{
    FORT_HOST_IPS_PURGE_ARG arg = {
        .keep_reauth = TRUE,
        .zones_mask = zones_mask,
        .now = (INT64) KeQueryInterruptTime(),
    };

    KIRQL oldIrql = ExAcquireSpinLockExclusive(&host_ips->lock);
    {
        fort_host_ips_purge_locked(host_ips, &arg);
    }
    ExReleaseSpinLockExclusive(&host_ips->lock, oldIrql);
}

FORT_API UINT32 fort_host_ips_zones_mask(PFORT_HOST_IPS host_ips, PCFORT_CONF_META_CONN conn)
{
    if (host_ips->count == 0)
        return 0;

    const FORT_HOST_IP key = fort_host_ip_conn_key(conn);
    const tommy_hash_t hash = fort_host_ip_hash(&key);
    const INT64 now = (INT64) KeQueryInterruptTime();

    UINT32 zones_mask = 0;

    KIRQL oldIrql = ExAcquireSpinLockShared(&host_ips->lock);
    {
        PCFORT_HOST_IP host_ip =
                tommy_hashdyn_search(&host_ips->ips_map, &fort_host_ip_cmp, &key, hash);

        if (host_ip != NULL && !fort_host_ip_expired(host_ip, conn->is_reauth, now)) {
            zones_mask = host_ip->zones_mask;
        }
    }
    ExReleaseSpinLockShared(&host_ips->lock, oldIrql);

    return zones_mask;
}

/* Is the bucket's node the not expired IP of a name of the host() filter */
static BOOL fort_host_ip_filter_matched(
        const tommy_hashdyn_node *node, PCFORT_HOST_IPS_FILTER_ARG arg)
{
    if (node->index != arg->hash)
        return FALSE;

    PCFORT_HOST_IP host_ip = node->data;

    if (!host_ip->isFilter || fort_host_ip_addr_cmp(&arg->key, host_ip) != 0)
        return FALSE;

    if (fort_host_ip_expired(host_ip, arg->is_reauth, arg->now))
        return FALSE;

    return fort_conf_host_name_included(arg->host, host_ip->name_id);
}

static BOOL fort_host_ips_filter_included_locked(
        PFORT_HOST_IPS host_ips, PCFORT_HOST_IPS_FILTER_ARG arg)
{
    const tommy_hashdyn_node *node = tommy_hashdyn_bucket(&host_ips->ips_map, arg->hash);

    for (; node != NULL; node = node->next) {
        if (fort_host_ip_filter_matched(node, arg))
            return TRUE;
    }

    return FALSE;
}

FORT_API BOOL fort_host_ips_filter_included(
        PFORT_HOST_IPS host_ips, PCFORT_CONF_META_CONN conn, PCFORT_CONF_RULE_FILTER_HOST host)
{
    if (host_ips->count == 0)
        return FALSE;

    FORT_HOST_IPS_FILTER_ARG arg = {
        .is_reauth = conn->is_reauth,
        .now = (INT64) KeQueryInterruptTime(),
        .host = host,
        .key = fort_host_ip_conn_key(conn),
    };
    arg.hash = fort_host_ip_hash(&arg.key);

    BOOL included;

    KIRQL oldIrql = ExAcquireSpinLockShared(&host_ips->lock);
    {
        included = fort_host_ips_filter_included_locked(host_ips, &arg);
    }
    ExReleaseSpinLockShared(&host_ips->lock, oldIrql);

    return included;
}
