#ifndef FORTCONF_H
#define FORTCONF_H

#include "common.h"

#define FORT_CONF_IP_MAX              (2 * 1024 * 1024)
#define FORT_CONF_IP4_ARR_SIZE(n)     ((n) * sizeof(UINT32))
#define FORT_CONF_IP6_ARR_SIZE(n)     ((n) * sizeof(ip6_addr_t))
#define FORT_CONF_IP4_RANGE_SIZE(n)   (FORT_CONF_IP4_ARR_SIZE(n) * 2)
#define FORT_CONF_IP6_RANGE_SIZE(n)   (FORT_CONF_IP6_ARR_SIZE(n) * 2)
#define FORT_CONF_RULE_MAX            1024
#define FORT_CONF_RULE_SET_MAX        32
#define FORT_CONF_RULE_DEPTH_MAX      4
#define FORT_CONF_RULE_SET_DEPTH_MAX  8
#define FORT_CONF_ZONE_MAX            32
#define FORT_CONF_GROUP_MAX           16
#define FORT_CONF_APPS_LEN_MAX        (64 * 1024 * 1024)
#define FORT_CONF_APP_PATH_MAX        1024
#define FORT_CONF_APP_PATH_MAX_SIZE   (FORT_CONF_APP_PATH_MAX * sizeof(WCHAR))
#define FORT_CONF_STR_ALIGN           4
#define FORT_CONF_STR_HEADER_SIZE(n)  (((n) + 1) * sizeof(UINT32))
#define FORT_CONF_STR_DATA_SIZE(size) FORT_ALIGN_SIZE((size), FORT_CONF_STR_ALIGN)

typedef struct fort_conf_flags
{
    UINT32 boot_filter : 1;
    UINT32 filter_enabled : 1;
    UINT32 filter_locals : 1;

    UINT32 block_traffic : 1;
    UINT32 block_inet_traffic : 1;

    UINT32 allow_all_new : 1;
    UINT32 ask_to_connect : 1;
    UINT32 app_block_all : 1;
    UINT32 app_allow_all : 1;

    UINT32 log_stat : 1;
    UINT32 log_stat_no_filter : 1;
    UINT32 log_blocked : 1;

    UINT32 log_allowed_ip : 1;
    UINT32 log_blocked_ip : 1;
    UINT32 log_alerted_blocked_ip : 1;

    UINT32 reserved : 1; /* DUMMY */

    UINT32 group_bits : 16;
} FORT_CONF_FLAGS, *PFORT_CONF_FLAGS;

typedef struct fort_service_info
{
    UINT32 process_id;

    UINT16 name_len;
    WCHAR name[2];
} FORT_SERVICE_INFO, *PFORT_SERVICE_INFO;

typedef struct fort_service_info_list
{
    UINT16 services_n;

    FORT_SERVICE_INFO data[1];
} FORT_SERVICE_INFO_LIST, *PFORT_SERVICE_INFO_LIST;

#define FORT_SERVICE_INFO_NAME_OFF      offsetof(FORT_SERVICE_INFO, name)
#define FORT_SERVICE_INFO_LIST_DATA_OFF offsetof(FORT_SERVICE_INFO_LIST, data)
#define FORT_SERVICE_INFO_NAME_MAX      256
#define FORT_SERVICE_INFO_NAME_MAX_SIZE (FORT_SERVICE_INFO_NAME_MAX * sizeof(WCHAR))
#define FORT_SERVICE_INFO_MAX_SIZE      (FORT_SERVICE_INFO_NAME_OFF + FORT_SERVICE_INFO_NAME_MAX_SIZE)
#define FORT_SERVICE_INFO_LIST_MIN_SIZE                                                            \
    (FORT_SERVICE_INFO_LIST_DATA_OFF + FORT_SERVICE_INFO_MAX_SIZE)

typedef struct fort_conf_port_list
{
    UINT8 port_n;
    UINT8 pair_n;

    UINT16 port[1];
} FORT_CONF_PORT_LIST, *PFORT_CONF_PORT_LIST;

typedef struct fort_conf_addr4_list
{
    UINT32 ip_n;
    UINT32 pair_n;

    UINT32 ip[1];
} FORT_CONF_ADDR4_LIST, *PFORT_CONF_ADDR4_LIST;

typedef struct fort_conf_addr6_list
{
    UINT32 ip_n;
    UINT32 pair_n;

    ip6_addr_t ip[1];
} FORT_CONF_ADDR6_LIST, *PFORT_CONF_ADDR6_LIST;

typedef struct fort_conf_addr_group
{
    UINT32 include_all : 1;
    UINT32 exclude_all : 1;
    UINT32 include_is_empty : 1;
    UINT32 exclude_is_empty : 1;

    UINT32 include_zones;
    UINT32 exclude_zones;

    UINT32 exclude_off;

    char data[4];
} FORT_CONF_ADDR_GROUP, *PFORT_CONF_ADDR_GROUP;

#define FORT_RULE_FLAG_ADDRESS    0x01
#define FORT_RULE_FLAG_PORT       0x02
#define FORT_RULE_FLAG_PROTO_TCP  0x10
#define FORT_RULE_FLAG_PROTO_UDP  0x20
#define FORT_RULE_FLAG_PROTO_MASK 0xF0

typedef struct fort_conf_rule_expr
{
    UINT8 expr_begin : 1;
    UINT8 expr_end : 1;
    UINT8 expr_or : 1;
    UINT8 expr_local : 1; // Local Address/Port

    UINT8 has_ip6_list : 1;

    UINT8 flags;
} FORT_CONF_RULE_EXPR, *PFORT_CONF_RULE_EXPR;

typedef struct fort_conf_rule_zones
{
    UINT32 accept_zones;
    UINT32 reject_zones;
} FORT_CONF_RULE_ZONES, *PFORT_CONF_RULE_ZONES;

typedef struct fort_conf_rule
{
    UINT8 enabled : 1;
    UINT8 blocked : 1;
    UINT8 exclusive : 1;

    UINT8 has_zones : 1;
    UINT8 has_expr : 1;

    UINT8 set_count;
} FORT_CONF_RULE, *PFORT_CONF_RULE;

typedef struct fort_conf_rules
{
    UINT16 max_rule_id;

    char data[4];
} FORT_CONF_RULES, *PFORT_CONF_RULES;

typedef struct fort_conf_rule_flag
{
    UINT16 rule_id;
    UCHAR enabled;
} FORT_CONF_RULE_FLAG, *PFORT_CONF_RULE_FLAG;

#define FORT_CONF_RULES_DATA_OFF                  offsetof(FORT_CONF_RULES, data)
#define FORT_CONF_RULES_OFFSETS_SIZE(max_rule_id) ((max_rule_id + 1) * sizeof(UINT32))
#define FORT_CONF_RULE_SIZE(rule)                                                                  \
    (sizeof(FORT_CONF_RULE) + ((rule)->has_zones ? sizeof(FORT_CONF_RULE_ZONES) : 0)               \
            + (rule)->set_count * sizeof(UINT16))

typedef struct fort_conf_zones
{
    UINT32 mask;
    UINT32 enabled_mask;

    UINT32 addr_off[FORT_CONF_ZONE_MAX];

    char data[4];
} FORT_CONF_ZONES, *PFORT_CONF_ZONES;

typedef struct fort_conf_zone_flag
{
    UCHAR zone_id;
    UCHAR enabled;
} FORT_CONF_ZONE_FLAG, *PFORT_CONF_ZONE_FLAG;

typedef struct fort_traf
{
    union {
        UINT64 v;

        struct
        {
            UINT32 in_bytes;
            UINT32 out_bytes;
        };
    };
} FORT_TRAF, *PFORT_TRAF;

typedef struct fort_time
{
    union {
        UINT16 v;

        struct
        {
            UCHAR hour;
            UCHAR minute;
        };
    };
} FORT_TIME, *PFORT_TIME;

typedef struct fort_period
{
    union {
        UINT32 v;

        struct
        {
            FORT_TIME from;
            FORT_TIME to;
        };
    };
} FORT_PERIOD, *PFORT_PERIOD;

typedef struct fort_app_flags
{
    union {
        UINT16 v;

        struct
        {
            UINT16 group_index : 5;
            UINT16 use_group_perm : 1;

            UINT16 apply_child : 1;
            UINT16 kill_child : 1;
            UINT16 lan_only : 1;
            UINT16 log_blocked : 1;
            UINT16 log_conn : 1;

            UINT16 blocked : 1;
            UINT16 kill_process : 1;
            UINT16 alerted : 1;
            UINT16 is_new : 1;
            UINT16 found : 1;
        };
    };
} FORT_APP_FLAGS, *PFORT_APP_FLAGS;

typedef struct fort_app_data
{
    FORT_APP_FLAGS flags;

    UINT16 rule_id;

    UINT16 accept_zones;
    UINT16 reject_zones;
} FORT_APP_DATA, *PFORT_APP_DATA;

typedef struct fort_app_entry
{
    FORT_APP_DATA app_data;

    UINT16 path_len;

    WCHAR path[2];
} FORT_APP_ENTRY, *PFORT_APP_ENTRY;

#define FORT_CONF_APP_ENTRY_PATH_OFF offsetof(FORT_APP_ENTRY, path)
#define FORT_CONF_APP_ENTRY_SIZE(path_len)                                                         \
    (FORT_CONF_APP_ENTRY_PATH_OFF + (path_len) + sizeof(WCHAR)) /* include terminating zero */

typedef struct fort_speed_limit
{
    UINT16 plr; /* packet loss rate in 1/100% (0-10000, i.e. 10% packet loss = 1000) */
    UINT32 latency_ms; /* latency in milliseconds */
    UINT32 buffer_bytes; /* size of packet buffer in bytes (150,000 is the dummynet's default) */
    UINT64 bps; /* bandwidth in bytes per second */
} FORT_SPEED_LIMIT, *PFORT_SPEED_LIMIT;

typedef struct fort_conf_group
{
    UINT16 group_bits;

    UINT16 log_blocked;
    UINT16 log_conn;

    UINT16 limit_bits;
    UINT32 limit_io_bits;

    FORT_SPEED_LIMIT limits[FORT_CONF_GROUP_MAX * 2]; /* in/out-bound pairs */
} FORT_CONF_GROUP, *PFORT_CONF_GROUP;

typedef struct fort_conf
{
    FORT_CONF_FLAGS flags;

    UCHAR app_periods_n;

    UCHAR proc_wild : 1;

    UINT16 wild_apps_n;
    UINT16 prefix_apps_n;
    UINT16 exe_apps_n;

    UINT32 app_perms_block_mask;
    UINT32 app_perms_allow_mask;

    UINT32 addr_groups_off;

    UINT32 app_periods_off;

    UINT32 wild_apps_off;
    UINT32 prefix_apps_off;
    UINT32 exe_apps_off;

    char data[4];
} FORT_CONF, *PFORT_CONF;

typedef struct fort_conf_version
{
    UINT16 driver_version;
} FORT_CONF_VERSION, *PFORT_CONF_VERSION;

typedef struct fort_conf_io
{
    FORT_CONF_GROUP conf_group;

    FORT_CONF conf;
} FORT_CONF_IO, *PFORT_CONF_IO;

#define FORT_CONF_DATA_OFF       offsetof(FORT_CONF, data)
#define FORT_CONF_IO_CONF_OFF    offsetof(FORT_CONF_IO, conf)
#define FORT_CONF_ADDR4_LIST_OFF offsetof(FORT_CONF_ADDR4_LIST, ip)
#define FORT_CONF_ADDR6_LIST_OFF offsetof(FORT_CONF_ADDR6_LIST, ip)
#define FORT_CONF_ADDR_GROUP_OFF offsetof(FORT_CONF_ADDR_GROUP, data)
#define FORT_CONF_ZONES_DATA_OFF offsetof(FORT_CONF_ZONES, data)

#define FORT_CONF_ADDR4_LIST_SIZE(ip_n, pair_n)                                                    \
    (FORT_CONF_ADDR4_LIST_OFF + FORT_CONF_IP4_ARR_SIZE(ip_n) + FORT_CONF_IP4_RANGE_SIZE(pair_n))

#define FORT_CONF_ADDR6_LIST_SIZE(ip_n, pair_n)                                                    \
    (FORT_CONF_ADDR6_LIST_OFF + FORT_CONF_IP6_ARR_SIZE(ip_n) + FORT_CONF_IP6_RANGE_SIZE(pair_n))

#define FORT_CONF_ADDR_LIST_SIZE(ip4_n, pair4_n, ip6_n, pair6_n)                                   \
    (FORT_CONF_ADDR4_LIST_SIZE(ip4_n, pair4_n) + FORT_CONF_ADDR6_LIST_SIZE(ip6_n, pair6_n))

typedef FORT_APP_DATA fort_conf_app_exe_find_func(
        const PFORT_CONF conf, PVOID context, const PVOID path, UINT32 path_len);

typedef BOOL fort_conf_zones_ip_included_func(
        void *ctx, UINT32 zones_mask, const UINT32 *remote_ip, BOOL isIPv6);

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API BOOL is_time_in_period(FORT_TIME time, FORT_PERIOD period);

FORT_API BOOL fort_conf_ip_inlist(
        const UINT32 *ip, const PFORT_CONF_ADDR4_LIST addr_list, BOOL isIPv6);

FORT_API PFORT_CONF_ADDR_GROUP fort_conf_addr_group_ref(
        const PFORT_CONF conf, int addr_group_index);

#define fort_conf_addr_group_include_list_ref(addr_group)                                          \
    ((PFORT_CONF_ADDR4_LIST) (addr_group)->data)

#define fort_conf_addr_group_exclude_list_ref(addr_group)                                          \
    ((PFORT_CONF_ADDR4_LIST) ((addr_group)->data + (addr_group)->exclude_off))

FORT_API BOOL fort_conf_ip_included(const PFORT_CONF conf,
        fort_conf_zones_ip_included_func zone_func, void *ctx, const UINT32 *remote_ip, BOOL isIPv6,
        int addr_group_index);

#define fort_conf_ip_is_inet(conf, zone_func, ctx, remote_ip, isIPv6)                              \
    fort_conf_ip_included((conf), (zone_func), (ctx), (remote_ip), isIPv6, /*addr_group_index=*/0)

#define fort_conf_ip_inet_included(conf, zone_func, ctx, remote_ip, isIPv6)                        \
    fort_conf_ip_included((conf), (zone_func), (ctx), (remote_ip), isIPv6, /*addr_group_index=*/1)

FORT_API BOOL fort_conf_app_exe_equal(
        const PFORT_APP_ENTRY app_entry, const PVOID path, UINT32 path_len);

FORT_API FORT_APP_DATA fort_conf_app_exe_find(
        const PFORT_CONF conf, PVOID context, const PVOID path, UINT32 path_len);

FORT_API FORT_APP_DATA fort_conf_app_find(const PFORT_CONF conf, const PVOID path, UINT32 path_len,
        fort_conf_app_exe_find_func *exe_find_func, PVOID exe_context);

FORT_API BOOL fort_conf_app_blocked(
        const PFORT_CONF conf, FORT_APP_FLAGS app_flags, INT8 *block_reason);

FORT_API UINT16 fort_conf_app_period_bits(const PFORT_CONF conf, FORT_TIME time, int *periods_n);

FORT_API void fort_conf_app_perms_mask_init(PFORT_CONF conf, UINT32 group_bits);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTCONF_H
