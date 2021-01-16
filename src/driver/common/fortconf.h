#ifndef FORTCONF_H
#define FORTCONF_H

#include "common.h"

#define FORT_CONF_IP_MAX             (10 * 1024 * 1024)
#define FORT_CONF_IP_ARR_SIZE(n)     ((n) * sizeof(UINT32))
#define FORT_CONF_IP_RANGE_SIZE(n)   (FORT_CONF_IP_ARR_SIZE(n) * 2)
#define FORT_CONF_ZONE_MAX           32
#define FORT_CONF_GROUP_MAX          16
#define FORT_CONF_APPS_LEN_MAX       (64 * 1024 * 1024)
#define FORT_CONF_APP_PATH_MAX       (2 * 1024)
#define FORT_CONF_STR_ALIGN          4
#define FORT_CONF_STR_HEADER_SIZE(n) (((n) + 1) * sizeof(UINT32))
#define FORT_CONF_STR_DATA_SIZE(size)                                                              \
    ((((size) + (FORT_CONF_STR_ALIGN - 1)) & ~(FORT_CONF_STR_ALIGN - 1)))
#define FORT_CONF_APP_ENTRY_SIZE(len)                                                              \
    (sizeof(FORT_APP_ENTRY) + (len) + sizeof(wchar_t)) /* include terminating zero */

typedef struct fort_conf_flags
{
    UINT32 prov_boot : 1;
    UINT32 filter_enabled : 1;
    UINT32 filter_locals : 1;
    UINT32 stop_traffic : 1;
    UINT32 stop_inet_traffic : 1;
    UINT32 allow_all_new : 1;
    UINT32 app_block_all : 1;
    UINT32 app_allow_all : 1;
    UINT32 log_blocked : 1;
    UINT32 log_stat : 1;
    UINT32 log_allowed_ip : 1;
    UINT32 log_blocked_ip : 1;

    UINT32 group_bits : 16;
} FORT_CONF_FLAGS, *PFORT_CONF_FLAGS;

static_assert(sizeof(FORT_CONF_FLAGS) == 4, "FORT_CONF_FLAGS is not 32 bits");

typedef struct fort_conf_addr_list
{
    UINT32 ip_n;
    UINT32 pair_n;

    UINT32 ip[1];
} FORT_CONF_ADDR_LIST, *PFORT_CONF_ADDR_LIST;

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
            UCHAR group_index;
            UCHAR use_group_perm : 1;
            UCHAR blocked : 1;
            UCHAR alerted : 1;
            UCHAR is_new : 1;
            UCHAR found : 1;
        };
    };
} FORT_APP_FLAGS, *PFORT_APP_FLAGS;

typedef struct fort_app_entry
{
    union {
        UINT32 v;

        struct
        {
            UINT16 path_len;
            FORT_APP_FLAGS flags;
        };
    };
} FORT_APP_ENTRY, *PFORT_APP_ENTRY;

typedef struct fort_conf_group
{
    UINT16 fragment_bits;

    UINT16 limit_bits;
    UINT32 limit_2bits;

    FORT_TRAF limits[FORT_CONF_GROUP_MAX]; /* Bytes per 0.5 sec. */
} FORT_CONF_GROUP, *PFORT_CONF_GROUP;

typedef struct fort_conf
{
    FORT_CONF_FLAGS flags;

    UCHAR app_periods_n;

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
#define FORT_CONF_ADDR_LIST_OFF  offsetof(FORT_CONF_ADDR_LIST, ip)
#define FORT_CONF_ADDR_GROUP_OFF offsetof(FORT_CONF_ADDR_GROUP, data)
#define FORT_CONF_ZONES_DATA_OFF offsetof(FORT_CONF_ZONES, data)

#define FORT_CONF_ADDR_LIST_SIZE(ip_n, pair_n)                                                     \
    (FORT_CONF_ADDR_LIST_OFF + FORT_CONF_IP_ARR_SIZE(ip_n) + FORT_CONF_IP_RANGE_SIZE(pair_n))

typedef FORT_APP_FLAGS fort_conf_app_exe_find_func(
        const PFORT_CONF conf, const char *path, UINT32 path_len);

typedef BOOL fort_conf_zones_ip_included_func(void *ctx, UINT32 zones_mask, UINT32 remote_ip);

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API int bit_scan_forward(unsigned long mask);

FORT_API BOOL is_time_in_period(FORT_TIME time, FORT_PERIOD period);

FORT_API BOOL fort_conf_ip_inlist(UINT32 ip, const PFORT_CONF_ADDR_LIST addr_list);

FORT_API PFORT_CONF_ADDR_GROUP fort_conf_addr_group_ref(
        const PFORT_CONF conf, int addr_group_index);

#define fort_conf_addr_group_include_list_ref(addr_group) ((PFORT_CONF_ADDR_LIST)(addr_group)->data)

#define fort_conf_addr_group_exclude_list_ref(addr_group)                                          \
    ((PFORT_CONF_ADDR_LIST)((addr_group)->data + (addr_group)->exclude_off))

FORT_API BOOL fort_conf_ip_included(const PFORT_CONF conf,
        fort_conf_zones_ip_included_func zone_func, void *ctx, UINT32 remote_ip,
        int addr_group_index);

#define fort_conf_ip_is_inet(conf, zone_func, ctx, remote_ip)                                      \
    fort_conf_ip_included((conf), (zone_func), (ctx), (remote_ip), 0)

#define fort_conf_ip_inet_included(conf, zone_func, ctx, remote_ip)                                \
    fort_conf_ip_included((conf), (zone_func), (ctx), (remote_ip), 1)

FORT_API BOOL fort_conf_app_exe_equal(PFORT_APP_ENTRY app_entry, const char *path, UINT32 path_len);

FORT_API FORT_APP_FLAGS fort_conf_app_exe_find(
        const PFORT_CONF conf, const char *path, UINT32 path_len);

FORT_API FORT_APP_FLAGS fort_conf_app_find(const PFORT_CONF conf, const char *path, UINT32 path_len,
        fort_conf_app_exe_find_func *exe_find_func);

FORT_API BOOL fort_conf_app_blocked(const PFORT_CONF conf, FORT_APP_FLAGS app_flags);

FORT_API UINT16 fort_conf_app_period_bits(const PFORT_CONF conf, FORT_TIME time, int *periods_n);

FORT_API void fort_conf_app_perms_mask_init(PFORT_CONF conf, UINT32 group_bits);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTCONF_H
