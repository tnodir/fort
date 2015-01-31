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

  UINT16 groups_n;
  UINT16 groups_len;

  UINT16 apps_n;
  UINT16 apps_len;

  UINT32 ip_include_off;
  UINT32 ip_exclude_off;
  UINT32 groups_off;
  UINT32 apps_off;

  UCHAR data[4];
} WIPF_CONF, *PWIPF_CONF;

#define WIPF_CONF_SIZE_MIN		offsetof(WIPF_CONF, data)
#define WIPF_CONF_IP_MAX		(1 * 1024 * 1024)
#define WIPF_CONF_GROUP_MAX		10
#define WIPF_CONF_GROUP_NAME_MAX	256
#define WIPF_CONF_GROUPS_LEN_MAX	(WIPF_CONF_GROUP_MAX * WIPF_CONF_GROUP_NAME_MAX)
#define WIPF_CONF_APPS_LEN_MAX		(64 * 1024)

#endif WIPFCONF_H
