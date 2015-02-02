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
  const UINT8 *p = (UINT8 *) conf;

  const BOOL ip_included = conf->ip_include_all ? TRUE
      : wipf_conf_ip_inrange(remote_ip, conf->ip_include_n,
                             (const UINT32 *) (p + conf->ip_from_include_off),
                             (const UINT32 *) (p + conf->ip_to_include_off));

  const BOOL ip_excluded = conf->ip_exclude_all ? TRUE
      : wipf_conf_ip_inrange(remote_ip, conf->ip_exclude_n,
                             (const UINT32 *) (p + conf->ip_from_exclude_off),
                             (const UINT32 *) (p + conf->ip_to_exclude_off));

  return ip_included && !ip_excluded;
}

static BOOL
wipf_conf_app_cmp (UINT32 path_len, const char *path, const char *apps,
                   const UINT32 *app_offp, BOOL is_less)
{
  const UINT32 app_off = *app_offp;
  const UINT32 app_len = *(app_offp + 1) - app_off;
  const char *app = apps + app_off;
  const BOOL len_is_less = (path_len < app_len);

  if (is_less) {
    const UINT32 len = len_is_less ? path_len : app_len;
    const int res = memcmp(path, app, len);

    return res ? res < 0 : len_is_less;
  }

  if (!len_is_less) {
    const WCHAR last = *((PWCH) (app + app_len) - 1);

    if (last == L'*') {
      path_len -= sizeof(WCHAR);  // partial comparison
    } else if (path_len != app_len) {
      return -1;
    }

    return !memcmp(path, app, path_len);
  }
  return -1;
}

static int
wipf_conf_app_index (UINT32 path_len, const char *path, UINT32 count,
                     const UINT32 *app_offsets)
{
  const char *apps;
  int beg, end;

  if (count == 0)
    return -1;

  apps = (const char *) (app_offsets + count);
  beg = 0, end = count - 1;

  do {
    const int mid = (beg + end) / 2;

    if (wipf_conf_app_cmp(path_len, path, apps, app_offsets + mid, TRUE)) {
      end = mid - 1;
    } else {
      if (beg == mid) {
        if (!wipf_conf_app_cmp(path_len, path, apps, app_offsets + end, TRUE)) {
          beg = end;
        }
        break;
      }
      beg = mid;
    }
  } while (beg < end);

  return wipf_conf_app_cmp(path_len, path, apps, app_offsets + beg, FALSE)
      ? beg : -1;
}

static BOOL
wipf_conf_app_blocked (const PWIPF_CONF conf,
                       UINT32 path_len, const char *path, BOOL *notify)
{
  const int app_index = wipf_conf_app_index(path_len, path, conf->apps_n,
      (const UINT32 *) ((UINT8 *) conf + conf->apps_off));

  //conf->app_block_all
  return FALSE;
}
