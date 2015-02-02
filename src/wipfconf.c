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
wipf_conf_app_blocked (const PWIPF_CONF conf,
                       UINT32 path_len, const char *path, BOOL *notify)
{
  //conf->app_block_all
  return FALSE;
}
