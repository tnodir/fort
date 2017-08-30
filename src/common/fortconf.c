/* Fort Firewall Driver Configuration */

#include "fortconf.h"


#ifndef FORT_DRIVER
#define fort_memcmp	memcmp
#else
static int
fort_memcmp (const char *p1, const char *p2, size_t len)
{
  const size_t n = RtlCompareMemory(p1, p2, len);
  return (n == len) ? 0 : (p1[n] - p2[n]);
}
#endif

static BOOL
fort_conf_ip_inrange (UINT32 ip, UINT32 count,
                      const UINT32 *iprange_from, const UINT32 *iprange_to)
{
  int low, high;

  if (count == 0)
    return FALSE;

  low = 0, high = count - 1;

  do {
    const int mid = (low + high) / 2;
    const UINT32 mid_ip = iprange_from[mid];

    if (ip < mid_ip)
      high = mid - 1;
    else if (ip > mid_ip)
      low = mid + 1;
    else
      return TRUE;
  } while (low <= high);

  return high >= 0 && ip >= iprange_from[high] && ip <= iprange_to[high];
}

static BOOL
fort_conf_ip_included (const PFORT_CONF conf, UINT32 remote_ip)
{
  const char *data = (const char *) conf + conf->data_off;

  const BOOL ip_included = conf->flags.ip_include_all ? TRUE
      : fort_conf_ip_inrange(remote_ip, conf->ip_include_n,
                             (const UINT32 *) (data + conf->ip_from_include_off),
                             (const UINT32 *) (data + conf->ip_to_include_off));

  const BOOL ip_excluded = conf->flags.ip_exclude_all ? TRUE
      : fort_conf_ip_inrange(remote_ip, conf->ip_exclude_n,
                             (const UINT32 *) (data + conf->ip_from_exclude_off),
                             (const UINT32 *) (data + conf->ip_to_exclude_off));

  return ip_included && !ip_excluded;
}

static int
fort_conf_app_cmp (UINT32 path_len, const char *path,
                   const char *apps, const UINT32 *app_offp)
{
  const UINT32 app_off = *app_offp;
  const UINT32 app_len = *(app_offp + 1) - app_off;
  const char *app = apps + app_off;

  if (path_len > app_len)
    path_len = app_len;

  return fort_memcmp(path, app, path_len);
}

static int
fort_conf_app_index (UINT32 path_len, const char *path, UINT32 count,
                     const UINT32 *app_offsets)
{
  const char *apps;
  int low, high;

  if (count == 0)
    return -1;

  apps = (const char *) (app_offsets + count + 1);
  low = 0, high = count - 1;

  do {
    const int mid = (low + high) / 2;
    const int res = fort_conf_app_cmp(path_len, path,
                                      apps, app_offsets + mid);

    if (res < 0)
      high = mid - 1;
    else if (res > 0)
      low = mid + 1;
    else
      return mid;
  } while (low <= high);

  return -1;
}

static BOOL
fort_conf_app_blocked (const PFORT_CONF conf,
                       UINT32 path_len, const char *path, BOOL *notify)
{
  const char *data = (const char *) conf + conf->data_off;
  const int app_index = fort_conf_app_index(path_len, path, conf->apps_n,
      (const UINT32 *) (data + conf->apps_off));
  const UINT32 *app_perms = (const UINT32 *) (data + conf->app_perms_off);
  const UINT32 app_perm = (app_index != -1) ? app_perms[app_index] : 0;

  const BOOL app_perm_blocked = (app_perm & conf->app_perms_block_mask);
  const BOOL app_blocked = conf->flags.app_block_all ? TRUE : app_perm_blocked;

  const BOOL app_perm_allowed = (app_perm & conf->app_perms_allow_mask);
  const BOOL app_allowed = conf->flags.app_allow_all ? TRUE : app_perm_allowed;

  *notify = app_blocked && !app_perm_blocked;

  return app_blocked && !app_allowed;
}

static void
fort_conf_app_perms_mask_init (PFORT_CONF conf)
{
  const UINT32 group_bits = conf->flags.group_bits;
  UINT32 perms_mask =
       (group_bits & 0x0001)        | ((group_bits & 0x0002) << 1)
    | ((group_bits & 0x0004) << 2)  | ((group_bits & 0x0008) << 3)
    | ((group_bits & 0x0010) << 4)  | ((group_bits & 0x0020) << 5)
    | ((group_bits & 0x0040) << 6)  | ((group_bits & 0x0080) << 7)
    | ((group_bits & 0x0100) << 8)  | ((group_bits & 0x0200) << 9)
    | ((group_bits & 0x0400) << 10) | ((group_bits & 0x0800) << 11)
    | ((group_bits & 0x1000) << 12) | ((group_bits & 0x2000) << 13)
    | ((group_bits & 0x4000) << 14) | ((group_bits & 0x8000) << 15);

  perms_mask |= perms_mask << 1;

  conf->app_perms_block_mask = (perms_mask & 0xAAAAAAAA);
  conf->app_perms_allow_mask = (perms_mask & 0x55555555);
}
