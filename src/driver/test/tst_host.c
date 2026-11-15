/* Fort Firewall Driver: Test: Hosts' IPs */

#include "test.h"

#include "../fortcnf_host.h"

#define TEST_NAME_ID 7

/* Adds the IP to the Zone 1 and to the host() filter's name */
static void test_host_ips_add_name(PFORT_HOST_IPS host_ips, UINT32 ip, UINT64 name_id, UINT32 ttl)
{
    UINT64 buf[(FORT_CONF_HOST_IPS_SIZE(1, 1, 0) + sizeof(UINT64) - 1) / sizeof(UINT64)];
    PFORT_CONF_HOST_IPS conf_ips = (PFORT_CONF_HOST_IPS) buf;

    *conf_ips = (FORT_CONF_HOST_IPS) {
        .name_n = 1,
        .ip4_n = 1,
        .zones_mask = 1,
        .ttl = ttl,
    };

    RtlCopyMemory(conf_ips->data, &name_id, sizeof(name_id));
    RtlCopyMemory(conf_ips->data + FORT_CONF_HOST_NAME_IDS_SIZE(1), &ip, sizeof(ip));

    assert(fort_host_ips_add(host_ips, conf_ips) == STATUS_SUCCESS);
}

static void test_host_ips_add(PFORT_HOST_IPS host_ips, UINT32 ip, UINT32 ttl)
{
    test_host_ips_add_name(host_ips, ip, TEST_NAME_ID, ttl);
}

#define TEST_FILTER_NAMES_MAX 3

static BOOL test_host_ips_filter_included(
        PFORT_HOST_IPS host_ips, PCFORT_CONF_META_CONN conn, const UINT64 *name_ids, UINT32 name_n)
{
    UINT64 buf[(FORT_CONF_RULE_FILTER_HOST_SIZE(TEST_FILTER_NAMES_MAX) + sizeof(UINT64) - 1)
            / sizeof(UINT64)];
    PFORT_CONF_RULE_FILTER_HOST host = (PFORT_CONF_RULE_FILTER_HOST) buf;

    assert(name_n <= TEST_FILTER_NAMES_MAX);

    host->name_n = name_n;
    RtlCopyMemory(host->name_ids, name_ids, FORT_CONF_HOST_NAME_IDS_SIZE(name_n));

    return fort_host_ips_filter_included(host_ips, conn, host);
}

static BOOL test_host_ips_name_included(PFORT_HOST_IPS host_ips, PCFORT_CONF_META_CONN conn)
{
    const UINT64 name_id = TEST_NAME_ID;

    return test_host_ips_filter_included(host_ips, conn, &name_id, 1);
}

static void test_host_ips_filter(void)
{
    FORT_HOST_IPS host_ips = { 0 };
    fort_host_ips_open(&host_ips);

    /* The IP of two names: the user-mode KeQueryInterruptTime() stands still, a TTL of 0 has
     * expired */
    test_host_ips_add_name(&host_ips, 0x01020304, /*name_id=*/7, /*ttl=*/1);
    test_host_ips_add_name(&host_ips, 0x01020304, /*name_id=*/9, /*ttl=*/0);

    FORT_CONF_META_CONN conn = { .remote_ip.v4 = 0x01020304 };

    /* A filter of the IP's name among others */
    const UINT64 names7[] = { 5, 7, 11 };
    assert(test_host_ips_filter_included(&host_ips, &conn, names7, 3));

    /* A filter of other names */
    const UINT64 names5[] = { 5, 11 };
    assert(!test_host_ips_filter_included(&host_ips, &conn, names5, 2));
    assert(!test_host_ips_filter_included(&host_ips, &conn, names5, 0));

    /* The expired name's IP is matched by an established connection's reauthorization only */
    const UINT64 names9[] = { 9 };
    assert(!test_host_ips_filter_included(&host_ips, &conn, names9, 1));

    conn.is_reauth = TRUE;
    assert(test_host_ips_filter_included(&host_ips, &conn, names9, 1));

    /* Another IP */
    conn = (FORT_CONF_META_CONN) { .remote_ip.v4 = 0x05060708 };
    assert(!test_host_ips_filter_included(&host_ips, &conn, names7, 3));

    fort_host_ips_close(&host_ips);

    printf("test_host_ips_filter: OK\n");
}

static void test_host_ips_reauth(void)
{
    FORT_HOST_IPS host_ips = { 0 };
    fort_host_ips_open(&host_ips);

    /* The user-mode KeQueryInterruptTime() stands still: a TTL of 0 has expired */
    test_host_ips_add(&host_ips, 0x01020304, /*ttl=*/0);
    test_host_ips_add(&host_ips, 0x05060708, /*ttl=*/1);

    FORT_CONF_META_CONN conn = { .remote_ip.v4 = 0x01020304 };

    /* The expired IP isn't matched by a new connection */
    assert(fort_host_ips_zones_mask(&host_ips, &conn) == 0);
    assert(!test_host_ips_name_included(&host_ips, &conn));

    /* The expired IP is matched by an established connection's reauthorization */
    conn.is_reauth = TRUE;
    assert(fort_host_ips_zones_mask(&host_ips, &conn) == 1);
    assert(test_host_ips_name_included(&host_ips, &conn));

    /* The purge keeps it */
    fort_host_ips_zones_keep(&host_ips, 1);
    assert(fort_host_ips_zones_mask(&host_ips, &conn) == 1);

    /* The not expired IP is matched by a new connection */
    conn = (FORT_CONF_META_CONN) { .remote_ip.v4 = 0x05060708 };
    assert(fort_host_ips_zones_mask(&host_ips, &conn) == 1);
    assert(test_host_ips_name_included(&host_ips, &conn));

    fort_host_ips_close(&host_ips);

    printf("test_host_ips_reauth: OK\n");
}

static void test_host_ips_purge(void)
{
    FORT_HOST_IPS host_ips = { 0 };
    fort_host_ips_open(&host_ips);

    /* Each IP is added to the Zones and to the host() filter */
    test_host_ips_add(&host_ips, 0x01020304, /*ttl=*/1);
    test_host_ips_add(&host_ips, 0x05060708, /*ttl=*/1);
    assert(host_ips.count == 4);

    /* The Zones' IPs are removed, the host() filter's ones are kept */
    fort_host_ips_zones_keep(&host_ips, 0);
    assert(host_ips.count == 2);

    const FORT_CONF_META_CONN conn = { .remote_ip.v4 = 0x05060708 };
    assert(fort_host_ips_zones_mask(&host_ips, &conn) == 0);
    assert(test_host_ips_name_included(&host_ips, &conn));

    fort_host_ips_close(&host_ips);

    printf("test_host_ips_purge: OK\n");
}

static NTSTATUS test_host_ips_add_zones(
        PFORT_HOST_IPS host_ips, UINT32 ip, UINT32 zones_mask, UINT32 ttl)
{
    char buf[FORT_CONF_HOST_IPS_SIZE(0, 1, 0)];
    PFORT_CONF_HOST_IPS conf_ips = (PFORT_CONF_HOST_IPS) buf;

    *conf_ips = (FORT_CONF_HOST_IPS) {
        .ip4_n = 1,
        .zones_mask = zones_mask,
        .ttl = ttl,
    };

    RtlCopyMemory(conf_ips->data, &ip, sizeof(ip));

    return fort_host_ips_add(host_ips, conf_ips);
}

static NTSTATUS test_host_ips_add_zone(PFORT_HOST_IPS host_ips, UINT32 ip)
{
    return test_host_ips_add_zones(host_ips, ip, /*zones_mask=*/1, /*ttl=*/1);
}

static void test_host_ip_expire(void *obj)
{
    PFORT_HOST_IP host_ip = obj;

    /* Expired for the reauthorization too */
    host_ip->expire_time = -(1LL << 62);
}

static void test_host_ip_ttl_expire(void *obj)
{
    PFORT_HOST_IP host_ip = obj;

    /* Expired by the TTL, not for the reauthorization */
    host_ip->expire_time = (INT64) KeQueryInterruptTime();
}

/* Expires every other IP for the reauthorization too, the rest by the TTL only */
static void test_host_ip_expire_odd(void *arg, void *obj)
{
    UINT32 *n = arg;

    if ((++*n & 1) != 0) {
        test_host_ip_expire(obj);
    } else {
        test_host_ip_ttl_expire(obj);
    }
}

static void test_host_ips_fill(PFORT_HOST_IPS host_ips, UINT32 *ip)
{
    while (test_host_ips_add_zone(host_ips, *ip) == STATUS_SUCCESS) {
        ++*ip;
    }
}

static void test_host_ips_reauth_zones(void)
{
    FORT_HOST_IPS host_ips = { 0 };
    fort_host_ips_open(&host_ips);

    /* The user-mode KeQueryInterruptTime() stands still: a TTL of 0 has expired */
    assert(test_host_ips_add_zones(&host_ips, 0x01020304, /*zones_mask=*/1, /*ttl=*/0)
            == STATUS_SUCCESS);

    /* The expired IP is added to another Zone */
    assert(test_host_ips_add_zones(&host_ips, 0x01020304, /*zones_mask=*/2, /*ttl=*/1)
            == STATUS_SUCCESS);

    const FORT_CONF_META_CONN conn = { .remote_ip.v4 = 0x01020304 };

    /* The old Zone is kept */
    assert(fort_host_ips_zones_mask(&host_ips, &conn) == 3);

    /* The removed Zone isn't matched */
    fort_host_ips_zones_keep(&host_ips, 2);
    assert(fort_host_ips_zones_mask(&host_ips, &conn) == 2);

    /* The IP expired for the reauthorization too is added to another Zone without the old one */
    tommy_hashdyn_foreach(&host_ips.ips_map, &test_host_ip_expire);
    assert(test_host_ips_add_zones(&host_ips, 0x01020304, /*zones_mask=*/4, /*ttl=*/1)
            == STATUS_SUCCESS);
    assert(fort_host_ips_zones_mask(&host_ips, &conn) == 4);

    fort_host_ips_close(&host_ips);

    printf("test_host_ips_reauth_zones: OK\n");
}

static void test_host_ips_full(void)
{
    FORT_HOST_IPS host_ips = { 0 };
    fort_host_ips_open(&host_ips);

    /* The first IP over the max purges the table, but no IP is expired */
    UINT32 ip = 1;
    test_host_ips_fill(&host_ips, &ip);
    const UINT32 count_max = host_ips.count;
    assert(count_max == ip - 1);

    /* The expired IPs aren't purged till the purge's period passes */
    tommy_hashdyn_foreach(&host_ips.ips_map, &test_host_ip_ttl_expire);
    assert(test_host_ips_add_zone(&host_ips, ip) == STATUS_INSUFFICIENT_RESOURCES);
    assert(host_ips.count == count_max);

    /* The user-mode KeQueryInterruptTime() stands still: the period passes by the reset.
     * No IP is expired for the reauthorization: the ones expired by the TTL are purged. */
    host_ips.purge_time = 0;
    assert(test_host_ips_add_zone(&host_ips, ip++) == STATUS_SUCCESS);
    assert(host_ips.count == 1);

    /* The IPs expired for the reauthorization are purged, the ones expired by the TTL are kept */
    test_host_ips_fill(&host_ips, &ip);
    assert(host_ips.count == count_max);

    UINT32 n = 0;
    tommy_hashdyn_foreach_arg(&host_ips.ips_map, &test_host_ip_expire_odd, &n);
    assert(n == count_max);

    host_ips.purge_time = 0;
    assert(test_host_ips_add_zone(&host_ips, ip) == STATUS_SUCCESS);
    assert(host_ips.count == count_max - (count_max + 1) / 2 + 1);

    fort_host_ips_close(&host_ips);

    printf("test_host_ips_full: OK\n");
}

static void tst_host_main(void)
{
    test_host_ips_reauth();
    test_host_ips_filter();
    test_host_ips_purge();
    test_host_ips_reauth_zones();
    test_host_ips_full();
}
