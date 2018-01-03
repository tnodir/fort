#ifndef FORTCONF_H
#define FORTCONF_H

#define FORT_CONF_IP_MAX		(1 * 1024 * 1024)
#define FORT_CONF_GROUP_MAX		16
#define FORT_CONF_APPS_LEN_MAX		(64 * 1024 * 1024)
#define FORT_CONF_APP_PATH_MAX		(2 * 1024)
#define FORT_CONF_STR_ALIGN		4
#define FORT_CONF_STR_HEADER_SIZE(n)	(((n) + 1) * sizeof(UINT32))
#define FORT_CONF_STR_DATA_SIZE(size)	((((size) + (FORT_CONF_STR_ALIGN - 1)) & ~(FORT_CONF_STR_ALIGN - 1)))

typedef struct fort_conf_flags {
  UINT32 prov_boot		: 1;
  UINT32 filter_enabled		: 1;
  UINT32 stop_traffic		: 1;
  UINT32 ip_include_all		: 1;
  UINT32 ip_exclude_all		: 1;
  UINT32 app_block_all		: 1;
  UINT32 app_allow_all		: 1;
  UINT32 log_blocked		: 1;
  UINT32 log_stat		: 1;
  UINT32 speed_limit		: 1;
  UINT32 _reserved_		: 6;
  UINT32 group_bits		: 16;
} FORT_CONF_FLAGS, *PFORT_CONF_FLAGS;

typedef struct fort_conf_limit {
  /* Bytes per 0.5 sec. */
  UINT32 in_bytes;
  UINT32 out_bytes;
} FORT_CONF_LIMIT, *PFORT_CONF_LIMIT;

typedef struct fort_conf {
  FORT_CONF_FLAGS flags;

  UINT16 data_off;

  UINT16 ip_include_n;
  UINT16 ip_exclude_n;

  UINT16 apps_n;

  UINT32 app_perms_block_mask;
  UINT32 app_perms_allow_mask;

  UINT32 ip_from_include_off;
  UINT32 ip_to_include_off;

  UINT32 ip_from_exclude_off;
  UINT32 ip_to_exclude_off;

  UINT32 app_groups_off;
  UINT32 app_perms_off;
  UINT32 apps_off;

  UCHAR data[4];
} FORT_CONF, *PFORT_CONF;

typedef struct fort_conf_io {
  UINT16 driver_version;

  FORT_CONF_LIMIT limits[FORT_CONF_GROUP_MAX];

  FORT_CONF conf;
} FORT_CONF_IO, *PFORT_CONF_IO;

#define FORT_CONF_DATA_OFF		offsetof(FORT_CONF, data)
#define FORT_CONF_IO_CONF_OFF		offsetof(FORT_CONF_IO, conf)

#endif FORTCONF_H
