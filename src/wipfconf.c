/* Windows IP Filter Configuration */

#include "wipfconf.h"


static BOOL
wipf_conf_ip_inrange (UINT32 ip, UINT32 count,
                      const UINT32 *iprange_from, const UINT32 *iprange_to)
{
  int beg, end;

  if (count == 0)
    return FALSE;

  beg = 0, end = count - 1;

  do {
    const int mid = (beg + end) / 2;

    if (ip < iprange_from[mid]) {
      end = mid - 1;
    } else {
      if (beg == mid) {
        if (ip >= iprange_from[end]) {
          beg = end;
        }
        break;
      }
      beg = mid;
    }
  } while (beg < end);

  return ip >= iprange_from[beg] && ip <= iprange_to[beg];
}

static BOOL
wipf_conf_ip_included (const PWIPF_CONF conf, UINT32 remote_ip)
{
  const char *data = (const char *) &conf->data;

  const BOOL ip_included = conf->ip_include_all ? TRUE
      : wipf_conf_ip_inrange(remote_ip, conf->ip_include_n,
                             (const UINT32 *) (data + conf->ip_from_include_off),
                             (const UINT32 *) (data + conf->ip_to_include_off));

  const BOOL ip_excluded = conf->ip_exclude_all ? TRUE
      : wipf_conf_ip_inrange(remote_ip, conf->ip_exclude_n,
                             (const UINT32 *) (data + conf->ip_from_exclude_off),
                             (const UINT32 *) (data + conf->ip_to_exclude_off));

  return ip_included && !ip_excluded;
}

static int
wipf_conf_app_cmp (UINT32 path_len, const char *path, const char *apps,
                   const UINT32 *app_offp, BOOL is_less)
{
  const UINT32 app_off = *app_offp;
  const UINT32 app_len = *(app_offp + 1) - app_off;
  const char *app = apps + app_off;

  if (is_less) {
    if (path_len > app_len)
      path_len = app_len;
  } else {
    if (path_len >= app_len) {
      const WCHAR app_last = *((LPWCH) (app + app_len) - 1);

      if (app_last == L'*') {
        path_len = app_len - sizeof(WCHAR);  // partial comparison
      } else if (path_len != app_len) {
        return -1;
      }
    } else {
      return 1;
    }
  }
  return memcmp(path, app, path_len);
}

static int
wipf_conf_app_index (UINT32 path_len, const char *path, UINT32 count,
                     const UINT32 *app_offsets)
{
  const char *apps;
  int beg, end;

  if (count == 0)
    return -1;

  apps = (const char *) (app_offsets + count + 1);
  beg = 0, end = count - 1;

  do {
    const int mid = (beg + end) / 2;

    if (wipf_conf_app_cmp(path_len, path, apps, app_offsets + mid, TRUE) < 0) {
      end = mid - 1;
    } else {
      if (beg == mid) {
        if (wipf_conf_app_cmp(path_len, path, apps, app_offsets + end, TRUE) >= 0) {
          beg = end;
        }
        break;
      }
      beg = mid;
    }
  } while (beg < end);

  return wipf_conf_app_cmp(path_len, path, apps, app_offsets + beg, FALSE)
      ? -1 : beg;
}

static BOOL
wipf_conf_app_blocked (const PWIPF_CONF conf,
                       UINT32 path_len, const char *path, BOOL *notify)
{
  const char *data = (const char *) &conf->data;
  const int app_index = wipf_conf_app_index(path_len, path, conf->apps_n,
      (const UINT32 *) (data + conf->apps_off));
  const UINT32 *apps_perms = (const UINT32 *) (data + conf->apps_perms_off);
  const UINT32 app_perms = (app_index != -1) ? apps_perms[app_index] : 0;

  const BOOL app_perm_blocked = (app_perms & conf->app_perms_block_mask);
  const BOOL app_blocked = conf->app_block_all ? TRUE : app_perm_blocked;

  const BOOL app_perm_allowed = (app_perms & conf->app_perms_allow_mask);
  const BOOL app_allowed = conf->app_allow_all ? TRUE : app_perm_allowed;

  *notify = app_blocked && !app_perm_blocked;

  return app_blocked && !app_allowed;
}

static void
wipf_conf_group_bits_set (const PWIPF_CONF conf, UINT32 group_bits)
{
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

  conf->group_bits = group_bits;

  conf->app_perms_block_mask = (perms_mask & 0xAAAAAAAA);
  conf->app_perms_allow_mask = (perms_mask & 0x55555555);
}
