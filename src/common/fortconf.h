#ifndef FORTCONF_H
#define FORTCONF_H

#define FORT_CONF_IP_MAX		(10 * 1024 * 1024)
#define FORT_CONF_IP_RANGE_SIZE(n)	((n) * sizeof(UINT32) * 2)
#define FORT_CONF_GROUP_MAX		16
#define FORT_CONF_APPS_LEN_MAX		(64 * 1024 * 1024)
#define FORT_CONF_APP_PATH_MAX		(2 * 1024)
#define FORT_CONF_STR_ALIGN		4
#define FORT_CONF_STR_HEADER_SIZE(n)	(((n) + 1) * sizeof(UINT32))
#define FORT_CONF_STR_DATA_SIZE(size)	((((size) + (FORT_CONF_STR_ALIGN - 1)) & ~(FORT_CONF_STR_ALIGN - 1)))
#define FORT_CONF_APP_ENTRY_SIZE(len)	(sizeof(FORT_APP_ENTRY) + (len) + sizeof(wchar_t))  /* include terminating zero */

typedef struct fort_conf_flags {
  UINT32 prov_boot		: 1;
  UINT32 filter_enabled		: 1;
  UINT32 filter_locals		: 1;
  UINT32 stop_traffic		: 1;
  UINT32 stop_inet_traffic	: 1;
  UINT32 app_block_all		: 1;
  UINT32 app_allow_all		: 1;
  UINT32 log_blocked		: 1;
  UINT32 log_stat		: 1;

  UINT32 group_bits		: 16;
} FORT_CONF_FLAGS, *PFORT_CONF_FLAGS;

typedef struct fort_conf_addr_group {
  UINT32 include_all		: 1;
  UINT32 exclude_all		: 1;

  UINT32 include_n;
  UINT32 exclude_n;

  UINT32 ip[2];
} FORT_CONF_ADDR_GROUP, *PFORT_CONF_ADDR_GROUP;

typedef struct fort_traf {
  union {
    UINT64 v;

    struct {
      UINT32 in_bytes;
      UINT32 out_bytes;
    };
  };
} FORT_TRAF, *PFORT_TRAF;

typedef struct fort_time {
  union {
    UINT16 v;

    struct {
      UCHAR hour;
      UCHAR minute;
    };
  };
} FORT_TIME, *PFORT_TIME;

typedef struct fort_period {
  union {
    UINT32 v;

    struct {
      FORT_TIME from;
      FORT_TIME to;
    };
  };
} FORT_PERIOD, *PFORT_PERIOD;

typedef struct fort_app_flags {
  union {
    UINT16 v;

    struct {
      UCHAR group_index;
      UCHAR use_group_perm  : 1;
      UCHAR blocked         : 1;
      UCHAR found           : 1;
    };
  };
} FORT_APP_FLAGS, *PFORT_APP_FLAGS;

typedef struct fort_app_entry {
  union {
    UINT32 v;

    struct {
      UINT16 path_len;
      FORT_APP_FLAGS flags;
    };
  };
} FORT_APP_ENTRY, *PFORT_APP_ENTRY;

typedef struct fort_conf_group {
  UINT16 fragment_bits;

  UINT16 limit_bits;
  UINT32 limit_2bits;

  FORT_TRAF limits[FORT_CONF_GROUP_MAX];  /* Bytes per 0.5 sec. */
} FORT_CONF_GROUP, *PFORT_CONF_GROUP;

typedef struct fort_conf {
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

typedef struct fort_conf_version {
  UINT16 driver_version;
} FORT_CONF_VERSION, *PFORT_CONF_VERSION;

typedef struct fort_conf_io {
  UINT16 driver_version;

  FORT_CONF_GROUP conf_group;

  FORT_CONF conf;
} FORT_CONF_IO, *PFORT_CONF_IO;

#define FORT_CONF_DATA_OFF		offsetof(FORT_CONF, data)
#define FORT_CONF_IO_CONF_OFF		offsetof(FORT_CONF_IO, conf)
#define FORT_CONF_ADDR_DATA_OFF		offsetof(FORT_CONF_ADDR_GROUP, ip)

typedef FORT_APP_FLAGS fort_conf_app_exe_find_func(
    const PFORT_CONF conf, UINT32 path_len, const char *path);

#endif FORTCONF_H
