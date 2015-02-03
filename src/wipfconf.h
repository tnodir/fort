#ifndef WIPFCONF_H
#define WIPFCONF_H

typedef struct wipf_conf {
  UINT32 ip_include_all		: 1;
  UINT32 ip_exclude_all		: 1;
  UINT32 app_log_blocked	: 1;
  UINT32 app_block_all		: 1;
  UINT32 app_allow_all		: 1;
  UINT32 group_bits		: 10;

  UINT16 ip_include_n;
  UINT16 ip_exclude_n;

  UINT16 apps_n;
  UINT16 groups_n;

  UINT32 app_perms_block_mask;
  UINT32 app_perms_allow_mask;

  UINT32 ip_from_include_off;
  UINT32 ip_to_include_off;

  UINT32 ip_from_exclude_off;
  UINT32 ip_to_exclude_off;

  UINT32 apps_perms_off;
  UINT32 apps_off;
  UINT32 groups_off;

  UCHAR data[4];
} WIPF_CONF, *PWIPF_CONF;

#define WIPF_CONF_SIZE_MIN		offsetof(WIPF_CONF, data)
#define WIPF_CONF_IP_MAX		(1 * 1024 * 1024)
#define WIPF_CONF_GROUP_MAX		16
#define WIPF_CONF_GROUP_NAME_MAX	256
#define WIPF_CONF_GROUPS_LEN_MAX	(WIPF_CONF_GROUP_MAX * WIPF_CONF_GROUP_NAME_MAX)
#define WIPF_CONF_APPS_LEN_MAX		(64 * 1024 * 1024)
#define WIPF_CONF_APP_PATH_MAX		(2 * 1024)
#define WIPF_CONF_STR_ALIGN		4

#endif WIPFCONF_H
