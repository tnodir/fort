#ifndef FORTCONF_H
#define FORTCONF_H

#include "common.h"

#define FORT_CONF_PROTO_MAX             255
#define FORT_CONF_PROTO_ARR_SIZE(n)     ((n) * sizeof(UINT8))
#define FORT_CONF_PROTO_RANGE_SIZE(n)   (FORT_CONF_PROTO_ARR_SIZE(n) * 2)
#define FORT_CONF_PORT_MAX              255
#define FORT_CONF_PORT_ARR_SIZE(n)      ((n) * sizeof(UINT16))
#define FORT_CONF_PORT_RANGE_SIZE(n)    (FORT_CONF_PORT_ARR_SIZE(n) * 2)
#define FORT_CONF_IP_MAX                (2 * 1024 * 1024)
#define FORT_CONF_IP4_ARR_SIZE(n)       ((n) * sizeof(UINT32))
#define FORT_CONF_IP6_ARR_SIZE(n)       ((n) * sizeof(ip6_addr_t))
#define FORT_CONF_IP4_RANGE_SIZE(n)     (FORT_CONF_IP4_ARR_SIZE(n) * 2)
#define FORT_CONF_IP6_RANGE_SIZE(n)     (FORT_CONF_IP6_ARR_SIZE(n) * 2)
#define FORT_CONF_RULE_MAX              1024
#define FORT_CONF_RULE_GLOBAL_MAX       64
#define FORT_CONF_RULE_SET_MAX          32
#define FORT_CONF_RULE_FILTER_DEPTH_MAX 7
#define FORT_CONF_RULE_SET_DEPTH_MAX    8
#define FORT_CONF_ZONE_MAX              32
#define FORT_CONF_GROUP_MAX             16
#define FORT_CONF_APPS_LEN_MAX          (64 * 1024 * 1024)
#define FORT_CONF_APP_PATH_MAX          1024
#define FORT_CONF_APP_PATH_MAX_SIZE     (FORT_CONF_APP_PATH_MAX * sizeof(WCHAR))
#define FORT_CONF_STR_ALIGN             4
#define FORT_CONF_STR_HEADER_SIZE(n)    (((n) + 1) * sizeof(UINT32))
#define FORT_CONF_STR_DATA_SIZE(size)   FORT_ALIGN_SIZE((size), FORT_CONF_STR_ALIGN)

typedef struct fort_conf_flags
{
    UINT32 boot_filter : 1;
    UINT32 filter_enabled : 1;
    UINT32 filter_locals : 1;
    UINT32 filter_local_net : 1;

    UINT32 block_traffic : 1;
    UINT32 block_lan_traffic : 1;
    UINT32 block_inet_traffic : 1;

    UINT32 allow_all_new : 1;
    UINT32 ask_to_connect : 1;
    UINT32 group_blocked : 1;

    UINT32 app_block_all : 1;
    UINT32 app_allow_all : 1;

    UINT32 log_stat : 1;
    UINT32 log_stat_no_filter : 1;
    UINT32 log_blocked : 1;

    UINT32 log_allowed_ip : 1;
    UINT32 log_blocked_ip : 1;
    UINT32 log_alerted_blocked_ip : 1;

    UINT32 reserved_flags : 14; /* not used */

    UINT16 group_bits;
    UINT16 reserved; /* not used */
} FORT_CONF_FLAGS, *PFORT_CONF_FLAGS;

typedef const FORT_CONF_FLAGS *PCFORT_CONF_FLAGS;

typedef struct fort_service_info
{
    UINT32 process_id;

    UINT16 name_len;
    WCHAR name[2];
} FORT_SERVICE_INFO, *PFORT_SERVICE_INFO;

typedef const FORT_SERVICE_INFO *PCFORT_SERVICE_INFO;

#define FORT_SERVICE_INFO_NAME_MAX      256
#define FORT_SERVICE_INFO_NAME_MAX_SIZE (FORT_SERVICE_INFO_NAME_MAX * sizeof(WCHAR))

#define FORT_SERVICE_INFO_NAME_OFF offsetof(FORT_SERVICE_INFO, name)
#define FORT_SERVICE_INFO_MAX_SIZE (FORT_SERVICE_INFO_NAME_OFF + FORT_SERVICE_INFO_NAME_MAX_SIZE)

typedef struct fort_service_info_list
{
    UINT16 services_n;

    FORT_SERVICE_INFO data[1];
} FORT_SERVICE_INFO_LIST, *PFORT_SERVICE_INFO_LIST;

typedef const FORT_SERVICE_INFO_LIST *PCFORT_SERVICE_INFO_LIST;

#define FORT_SERVICE_INFO_LIST_DATA_OFF offsetof(FORT_SERVICE_INFO_LIST, data)
#define FORT_SERVICE_INFO_LIST_MIN_SIZE                                                            \
    (FORT_SERVICE_INFO_LIST_DATA_OFF + FORT_SERVICE_INFO_MAX_SIZE)

typedef struct fort_conf_proto_list
{
    UINT8 proto_n;
    UINT8 pair_n;

    UINT8 proto[1];
} FORT_CONF_PROTO_LIST, *PFORT_CONF_PROTO_LIST;

typedef const FORT_CONF_PROTO_LIST *PCFORT_CONF_PROTO_LIST;

typedef struct fort_conf_port_list
{
    UINT8 port_n;
    UINT8 pair_n;

    UINT16 port[1];
} FORT_CONF_PORT_LIST, *PFORT_CONF_PORT_LIST;

typedef const FORT_CONF_PORT_LIST *PCFORT_CONF_PORT_LIST;

typedef struct fort_conf_addr_list
{
    UINT32 ip_n;
    UINT32 pair_n;

    UINT32 ip[1];
} FORT_CONF_ADDR_LIST, *PFORT_CONF_ADDR_LIST;

typedef const FORT_CONF_ADDR_LIST *PCFORT_CONF_ADDR_LIST;

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

typedef const FORT_CONF_ADDR_GROUP *PCFORT_CONF_ADDR_GROUP;

enum FORT_RULE_FILTER_TYPE {
    FORT_RULE_FILTER_TYPE_INVALID = -1,
    FORT_RULE_FILTER_TYPE_ADDRESS = 0,
    FORT_RULE_FILTER_TYPE_PORT,
    FORT_RULE_FILTER_TYPE_LOCAL_ADDRESS,
    FORT_RULE_FILTER_TYPE_LOCAL_PORT,
    FORT_RULE_FILTER_TYPE_PROTOCOL,
    FORT_RULE_FILTER_TYPE_DIRECTION,
    FORT_RULE_FILTER_TYPE_AREA,
    // Complex types
    FORT_RULE_FILTER_TYPE_PORT_TCP,
    FORT_RULE_FILTER_TYPE_PORT_UDP,
    // List types
    FORT_RULE_FILTER_TYPE_LIST_OR,
    FORT_RULE_FILTER_TYPE_LIST_AND,
};

enum {
    // Direction
    FORT_RULE_FILTER_DIRECTION_IN = (1 << 0),
    FORT_RULE_FILTER_DIRECTION_OUT = (1 << 1),
    // Area
    FORT_RULE_FILTER_AREA_LOCALHOST = (1 << 0),
    FORT_RULE_FILTER_AREA_LAN = (1 << 1),
    FORT_RULE_FILTER_AREA_INET = (1 << 2),
};

typedef struct fort_conf_rule_filter_flags
{
    UINT16 flags;
} FORT_CONF_RULE_FILTER_FLAGS, *PFORT_CONF_RULE_FILTER_FLAGS;

typedef const FORT_CONF_RULE_FILTER_FLAGS *PCFORT_CONF_RULE_FILTER_FLAGS;

typedef struct fort_conf_rule_filter
{
    UINT32 is_not : 1;
    UINT32 type : 4;
    UINT32 size : 27;
} FORT_CONF_RULE_FILTER, *PFORT_CONF_RULE_FILTER;

typedef const FORT_CONF_RULE_FILTER *PCFORT_CONF_RULE_FILTER;

typedef struct fort_conf_rule_zones
{
    UINT32 accept_mask;
    UINT32 reject_mask;
} FORT_CONF_RULE_ZONES, *PFORT_CONF_RULE_ZONES;

typedef const FORT_CONF_RULE_ZONES *PCFORT_CONF_RULE_ZONES;

typedef struct fort_conf_rule
{
    UINT8 enabled : 1;
    UINT8 blocked : 1;
    UINT8 exclusive : 1;

    UINT8 has_zones : 1;
    UINT8 has_filters : 1;

    UINT8 set_count;
} FORT_CONF_RULE, *PFORT_CONF_RULE;

typedef const FORT_CONF_RULE *PCFORT_CONF_RULE;

typedef struct fort_conf_rules_glob
{
    UINT16 pre_rule_id;
    UINT16 post_rule_id;
} FORT_CONF_RULES_GLOB, *PFORT_CONF_RULES_GLOB;

typedef struct fort_conf_rules
{
    UINT16 max_rule_id;

    FORT_CONF_RULES_GLOB glob;

    char data[4];
} FORT_CONF_RULES, *PFORT_CONF_RULES;

typedef const FORT_CONF_RULES *PCFORT_CONF_RULES;

typedef struct fort_conf_rule_flag
{
    UINT16 rule_id;
    UCHAR enabled;
} FORT_CONF_RULE_FLAG, *PFORT_CONF_RULE_FLAG;

typedef const FORT_CONF_RULE_FLAG *PCFORT_CONF_RULE_FLAG;

#define FORT_CONF_RULES_DATA_OFF offsetof(FORT_CONF_RULES, data)

#define FORT_CONF_RULES_OFFSETS_SIZE(max_rule_id) ((max_rule_id) * sizeof(UINT32))

#define FORT_CONF_RULES_SET_INDEXES_SIZE(n) ((n) * sizeof(UINT16))

#define FORT_CONF_RULE_SET_INDEXES_OFFSET(rule)                                                    \
    (sizeof(FORT_CONF_RULE) + ((rule)->has_zones ? sizeof(FORT_CONF_RULE_ZONES) : 0))

#define FORT_CONF_RULE_SIZE(rule)                                                                  \
    (FORT_CONF_RULE_SET_INDEXES_OFFSET(rule) + FORT_CONF_RULES_SET_INDEXES_SIZE((rule)->set_count))

typedef struct fort_conf_meta_conn
{
    UINT16 is_reauth : 1;
    UINT16 inbound : 1;
    UINT16 isIPv6 : 1;
    UINT16 is_tcp : 1;
    UINT16 is_loopback : 1;
    UINT16 is_broadcast : 1;
    UINT16 is_local_net : 1;
    UINT16 inherited : 1;
    UINT16 blocked : 1;

    UCHAR block_reason;

    UCHAR ip_proto;

    UINT16 local_port;
    UINT16 remote_port;

    UINT32 process_id;

    ip_addr_t local_ip;
    ip_addr_t remote_ip;

    FORT_APP_PATH path;
    FORT_APP_PATH real_path;
} FORT_CONF_META_CONN, *PFORT_CONF_META_CONN;

typedef const FORT_CONF_META_CONN *PCFORT_CONF_META_CONN;

typedef struct fort_conf_zones
{
    UINT32 mask;
    UINT32 enabled_mask;

    UINT32 addr_off[FORT_CONF_ZONE_MAX];

    char data[4];
} FORT_CONF_ZONES, *PFORT_CONF_ZONES;

typedef const FORT_CONF_ZONES *PCFORT_CONF_ZONES;

typedef struct fort_conf_zone_flag
{
    UCHAR zone_id;
    UCHAR enabled;
} FORT_CONF_ZONE_FLAG, *PFORT_CONF_ZONE_FLAG;

typedef const FORT_CONF_ZONE_FLAG *PCFORT_CONF_ZONE_FLAG;

typedef struct fort_conf_rules_rt
{
    const UINT32 *rule_offsets;
    const char *rules_data;

    PCFORT_CONF_ZONES zones;
} FORT_CONF_RULES_RT, *PFORT_CONF_RULES_RT;

typedef const FORT_CONF_RULES_RT *PCFORT_CONF_RULES_RT;

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

typedef struct fort_app_flags
{
    UINT16 group_index : 5;

    UINT16 apply_parent : 1;
    UINT16 apply_child : 1;
    UINT16 apply_spec_child : 1;
    UINT16 kill_child : 1;
    UINT16 lan_only : 1;
    UINT16 log_blocked : 1;
    UINT16 log_conn : 1;

    UINT16 blocked : 1;
    UINT16 kill_process : 1;

    UINT16 reserved : 2; /* not used */
} FORT_APP_FLAGS, *PFORT_APP_FLAGS;

typedef struct fort_app_data
{
    FORT_APP_FLAGS flags;

    UINT16 is_new : 1; /* can not replace an existing app data? */
    UINT16 found : 1; /* is app data not empty? */
    UINT16 alerted : 1;
    UINT16 rule_id : 13;

    UINT16 accept_zones;
    UINT16 reject_zones;
} FORT_APP_DATA, *PFORT_APP_DATA;

typedef struct fort_app_entry
{
    FORT_APP_DATA app_data;

    UINT16 path_len;

    WCHAR path[2];
} FORT_APP_ENTRY, *PFORT_APP_ENTRY;

typedef const FORT_APP_ENTRY *PCFORT_APP_ENTRY;

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

typedef const FORT_SPEED_LIMIT *PCFORT_SPEED_LIMIT;

typedef struct fort_conf_group
{
    UINT16 group_bits;

    UINT16 log_blocked;
    UINT16 log_conn;

    UINT16 limit_bits;
    UINT32 limit_io_bits;

    FORT_SPEED_LIMIT limits[FORT_CONF_GROUP_MAX * 2]; /* in/out-bound pairs */
} FORT_CONF_GROUP, *PFORT_CONF_GROUP;

typedef const FORT_CONF_GROUP *PCFORT_CONF_GROUP;

typedef struct fort_conf
{
    FORT_CONF_FLAGS flags;

    UCHAR proc_wild : 1; /* check also wildcard paths on process creation */

    UINT16 wild_apps_n;
    UINT16 prefix_apps_n;
    UINT16 exe_apps_n;

    UINT32 addr_groups_off;

    UINT32 wild_apps_off;
    UINT32 prefix_apps_off;
    UINT32 exe_apps_off;

    char data[4];
} FORT_CONF, *PFORT_CONF;

typedef const FORT_CONF *PCFORT_CONF;

typedef struct fort_conf_version
{
    UINT16 driver_version;
} FORT_CONF_VERSION, *PFORT_CONF_VERSION;

typedef const FORT_CONF_VERSION *PCFORT_CONF_VERSION;

typedef struct fort_conf_io
{
    FORT_CONF_GROUP conf_group;

    FORT_CONF conf;
} FORT_CONF_IO, *PFORT_CONF_IO;

typedef const FORT_CONF_IO *PCFORT_CONF_IO;

#define FORT_CONF_DATA_OFF       offsetof(FORT_CONF, data)
#define FORT_CONF_IO_CONF_OFF    offsetof(FORT_CONF_IO, conf)
#define FORT_CONF_PROTO_LIST_OFF offsetof(FORT_CONF_PROTO_LIST, proto)
#define FORT_CONF_PORT_LIST_OFF  offsetof(FORT_CONF_PORT_LIST, port)
#define FORT_CONF_ADDR_LIST_OFF  offsetof(FORT_CONF_ADDR_LIST, ip)
#define FORT_CONF_ADDR_GROUP_OFF offsetof(FORT_CONF_ADDR_GROUP, data)
#define FORT_CONF_ZONES_DATA_OFF offsetof(FORT_CONF_ZONES, data)

#define FORT_CONF_PROTO_LIST_SIZE(proto_n, pair_n)                                                 \
    (FORT_CONF_PROTO_LIST_OFF + FORT_CONF_PROTO_ARR_SIZE(proto_n)                                  \
            + FORT_CONF_PROTO_RANGE_SIZE(pair_n))

#define FORT_CONF_PORT_LIST_SIZE(port_n, pair_n)                                                   \
    (FORT_CONF_PORT_LIST_OFF + FORT_CONF_PORT_ARR_SIZE(port_n) + FORT_CONF_PORT_RANGE_SIZE(pair_n))

#define FORT_CONF_ADDR4_LIST_SIZE(ip_n, pair_n)                                                    \
    (FORT_CONF_ADDR_LIST_OFF + FORT_CONF_IP4_ARR_SIZE(ip_n) + FORT_CONF_IP4_RANGE_SIZE(pair_n))

#define FORT_CONF_ADDR6_LIST_SIZE(ip_n, pair_n)                                                    \
    (FORT_CONF_ADDR_LIST_OFF + FORT_CONF_IP6_ARR_SIZE(ip_n) + FORT_CONF_IP6_RANGE_SIZE(pair_n))

#define FORT_CONF_ADDR_LIST_SIZE(ip4_n, pair4_n, ip6_n, pair6_n)                                   \
    (FORT_CONF_ADDR4_LIST_SIZE(ip4_n, pair4_n) + FORT_CONF_ADDR6_LIST_SIZE(ip6_n, pair6_n))

typedef FORT_APP_DATA fort_conf_app_exe_find_func(
        PCFORT_CONF conf, PVOID context, PCFORT_APP_PATH path);

typedef BOOL fort_conf_zones_ip_included_func(
        void *ctx, UINT32 zones_mask, const ip_addr_t remote_ip, BOOL isIPv6);

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API int fort_mem_cmp(const void *p1, const void *p2, UINT32 len);

#define fort_ip6_cmp(l, r) fort_mem_cmp(l, r, sizeof(ip6_addr_t))

FORT_API BOOL fort_mem_eql(const void *p1, const void *p2, UINT32 len);

FORT_API BOOL fort_conf_ip_inlist(const ip_addr_t ip, PCFORT_CONF_ADDR_LIST addr_list, BOOL isIPv6);

FORT_API PCFORT_CONF_ADDR_GROUP fort_conf_addr_group_ref(PCFORT_CONF conf, int addr_group_index);

#define fort_conf_addr_group_include_list_ref(addr_group)                                          \
    ((PFORT_CONF_ADDR_LIST) (addr_group)->data)

#define fort_conf_addr_group_exclude_list_ref(addr_group)                                          \
    ((PFORT_CONF_ADDR_LIST) ((addr_group)->data + (addr_group)->exclude_off))

FORT_API BOOL fort_conf_ip_included(PCFORT_CONF conf, fort_conf_zones_ip_included_func zone_func,
        void *ctx, const ip_addr_t remote_ip, BOOL isIPv6, int addr_group_index);

#define fort_conf_ip_is_inet(conf, zone_func, ctx, remote_ip, isIPv6)                              \
    fort_conf_ip_included((conf), (zone_func), (ctx), (remote_ip), isIPv6, /*addr_group_index=*/0)

#define fort_conf_ip_inet_included(conf, zone_func, ctx, remote_ip, isIPv6)                        \
    fort_conf_ip_included((conf), (zone_func), (ctx), (remote_ip), isIPv6, /*addr_group_index=*/1)

FORT_API BOOL fort_conf_zones_ip_included(
        PCFORT_CONF_ZONES zones, UINT32 zones_mask, const ip_addr_t ip, BOOL isIPv6);

FORT_API BOOL fort_conf_zones_conn_blocked(PCFORT_CONF_ZONES zones, PCFORT_CONF_META_CONN conn,
        UINT32 reject_mask, UINT32 accept_mask);

FORT_API BOOL fort_conf_app_exe_equal(PCFORT_APP_ENTRY app_entry, PCFORT_APP_PATH path);

FORT_API FORT_APP_DATA fort_conf_app_exe_find(
        PCFORT_CONF conf, PVOID context, PCFORT_APP_PATH path);

FORT_API FORT_APP_DATA fort_conf_app_find(PCFORT_CONF conf, PCFORT_APP_PATH path,
        fort_conf_app_exe_find_func *exe_find_func, PVOID exe_context);

FORT_API BOOL fort_conf_app_group_blocked(const FORT_CONF_FLAGS conf_flags, FORT_APP_DATA app_data);

FORT_API BOOL fort_conf_rules_rt_conn_blocked(
        PCFORT_CONF_RULES_RT rules_rt, PCFORT_CONF_META_CONN conn, UINT16 rule_id);

FORT_API BOOL fort_conf_rules_conn_blocked(PCFORT_CONF_RULES rules, PCFORT_CONF_ZONES zones,
        PCFORT_CONF_META_CONN conn, UINT16 rule_id);

#define fort_conf_rules_rt_rule(rt, rule_id)                                                       \
    ((PFORT_CONF_RULE) ((rt)->rules_data + (rt)->rule_offsets[rule_id]))

FORT_API FORT_CONF_RULES_RT fort_conf_rules_rt_make(
        PCFORT_CONF_RULES rules, PCFORT_CONF_ZONES zones);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTCONF_H
