/* Fort Firewall Driver Configuration */

#include "fortconf.h"

#include "wildmatch.c"


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

static int
bit_scan_forward (unsigned long mask)
{
    unsigned long index;
    return _BitScanForward(&index, mask) ? index : -1;
}

static BOOL
is_time_in_period (FORT_TIME time, FORT_PERIOD period)
{
  const int x = time.hour * 60 + time.minute;
  const int from = period.from.hour * 60 + period.from.minute;
  const int to = period.to.hour * 60 + period.to.minute;

  return (from <= to ? (x >= from && x < (to - 1))
                     : (x >= from || x < (to - 1)));
}

static BOOL
fort_conf_ip_find (UINT32 ip, UINT32 count, const UINT32 *iparr, BOOL is_range)
{
  int low, high;

  if (count == 0)
    return FALSE;

  low = 0, high = count - 1;

  do {
    const int mid = (low + high) / 2;
    const UINT32 mid_ip = iparr[mid];

    if (ip < mid_ip)
      high = mid - 1;
    else if (ip > mid_ip)
      low = mid + 1;
    else
      return TRUE;
  } while (low <= high);

  if (!is_range)
    return FALSE;

  return high >= 0 && ip >= iparr[high]
    && ip <= iparr[count + high];
}

static BOOL
fort_conf_ip_inarr (UINT32 ip, UINT32 count, const UINT32 *iparr)
{
  return fort_conf_ip_find(ip, count, iparr, FALSE);
}

static BOOL
fort_conf_ip_inrange (UINT32 ip, UINT32 count, const UINT32 *iprange)
{
  return fort_conf_ip_find(ip, count, iprange, TRUE);
}

#define fort_conf_addr_list_ip_ref(addr_list) \
  (addr_list)->ip
#define fort_conf_addr_list_pair_ref(addr_list) \
  &(addr_list)->ip[(addr_list)->ip_n]

static BOOL
fort_conf_ip_inlist (UINT32 ip, const PFORT_CONF_ADDR_LIST addr_list)
{
  return fort_conf_ip_inarr(ip, addr_list->ip_n,
                            fort_conf_addr_list_ip_ref(addr_list))
    || fort_conf_ip_inrange(ip, addr_list->pair_n,
                            fort_conf_addr_list_pair_ref(addr_list));
}

static const PFORT_CONF_ADDR_GROUP
fort_conf_addr_group_ref (const PFORT_CONF conf, int addr_group_index)
{
  const UINT32 *addr_group_offsets = (const UINT32 *)
    (conf->data + conf->addr_groups_off);
  const char *addr_group_data = (const char *) addr_group_offsets;

  return (PFORT_CONF_ADDR_GROUP)
    (addr_group_data + addr_group_offsets[addr_group_index]);
}

#define fort_conf_addr_group_include_list_ref(addr_group) \
  ((PFORT_CONF_ADDR_LIST) (addr_group)->data)
#define fort_conf_addr_group_exclude_list_ref(addr_group) \
  ((PFORT_CONF_ADDR_LIST) ((addr_group)->data + (addr_group)->exclude_off))

static BOOL
fort_conf_ip_included (const PFORT_CONF conf,
                       fort_conf_zones_ip_included_func zone_func,
                       void *ctx, UINT32 remote_ip, int addr_group_index)
{
  const PFORT_CONF_ADDR_GROUP addr_group = fort_conf_addr_group_ref(
    conf, addr_group_index);

  const BOOL include_all = addr_group->include_all;
  const BOOL exclude_all = addr_group->exclude_all;

  const BOOL ip_included = include_all ? TRUE
    : ((!addr_group->include_is_empty
        && fort_conf_ip_inlist(remote_ip, fort_conf_addr_group_include_list_ref(addr_group)))
      || (zone_func != NULL && zone_func(ctx, addr_group->include_zones, remote_ip)));

  const BOOL ip_excluded = exclude_all ? TRUE
    : ((!addr_group->exclude_is_empty
        && fort_conf_ip_inlist(remote_ip, fort_conf_addr_group_exclude_list_ref(addr_group)))
      || (zone_func != NULL && zone_func(ctx, addr_group->exclude_zones, remote_ip)));

  return include_all ? !ip_excluded
    : (exclude_all ? ip_included
    : (ip_included && !ip_excluded));
}

#define fort_conf_ip_is_inet(conf, zones_func, ctx, remote_ip) \
  fort_conf_ip_included((conf), (zones_func), (ctx), (remote_ip), 0)

#define fort_conf_ip_inet_included(conf, zones_func, ctx, remote_ip) \
  fort_conf_ip_included((conf), (zones_func), (ctx), (remote_ip), 1)

static BOOL
fort_conf_app_exe_equal (PFORT_APP_ENTRY app_entry,
                         const char *path, UINT32 path_len)
{
  const char *app_path = (const char *) (app_entry + 1);
  const UINT32 app_path_len = app_entry->path_len;

  if (path_len != app_path_len)
    return FALSE;

  return fort_memcmp(path, app_path, path_len) == 0;
}

static FORT_APP_FLAGS
fort_conf_app_exe_find (const PFORT_CONF conf,
                        const char *path, UINT32 path_len)
{
  FORT_APP_FLAGS app_flags;
  const char *data;
  const char *app_entries;
  UINT16 count = conf->exe_apps_n;

  if (count == 0)
    goto not_found;

  data = conf->data;
  app_entries = (const char *) (data + conf->exe_apps_off);

  do {
    const PFORT_APP_ENTRY app_entry = (const PFORT_APP_ENTRY) app_entries;

    if (fort_conf_app_exe_equal(app_entry, path, path_len)) {
      app_flags = app_entry->flags;
      goto end;
    }

    app_entries += FORT_CONF_APP_ENTRY_SIZE(app_entry->path_len);
  } while (--count != 0);

 not_found:
  app_flags.v = 0;

 end:
  return app_flags;
}

static int
fort_conf_app_prefix_cmp (PFORT_APP_ENTRY app_entry,
                          const char *path, UINT32 path_len)
{
  const char *app_path = (const char *) (app_entry + 1);
  const UINT32 app_path_len = app_entry->path_len;

  if (path_len > app_path_len)
    path_len = app_path_len;

  return fort_memcmp(path, app_path, path_len);
}

static FORT_APP_FLAGS
fort_conf_app_prefix_find (const PFORT_CONF conf,
                           const char *path, UINT32 path_len)
{
  FORT_APP_FLAGS app_flags;
  const char *data;
  const UINT32 *app_offsets;
  const char *app_entries;
  const UINT16 count = conf->prefix_apps_n;
  int low, high;

  if (count == 0)
    goto not_found;

  data = conf->data;
  app_offsets = (const UINT32 *) (data + conf->prefix_apps_off);

  app_entries = (const char *) (app_offsets + count + 1);
  low = 0, high = count - 1;

  do {
    const int mid = (low + high) / 2;
    const UINT32 app_off = app_offsets[mid];
    const PFORT_APP_ENTRY app_entry = (PFORT_APP_ENTRY) (app_entries + app_off);
    const int res = fort_conf_app_prefix_cmp(app_entry, path, path_len);

    if (res < 0)
      high = mid - 1;
    else if (res > 0)
      low = mid + 1;
    else {
      app_flags = app_entry->flags;
      goto end;
    }
  } while (low <= high);

 not_found:
  app_flags.v = 0;

 end:
  return app_flags;
}

static FORT_APP_FLAGS
fort_conf_app_wild_find (const PFORT_CONF conf, const char *path)
{
  FORT_APP_FLAGS app_flags;
  const char *data;
  const char *app_entries;
  UINT16 count = conf->wild_apps_n;

  if (count == 0)
    goto not_found;

  data = conf->data;
  app_entries = (const char *) (data + conf->wild_apps_off);

  do {
    const PFORT_APP_ENTRY app_entry = (const PFORT_APP_ENTRY) app_entries;
    const WCHAR *app_path = (const WCHAR *) (app_entry + 1);
    const int res = wildmatch(app_path, (const WCHAR *) path);

    if (res == WM_MATCH) {
      app_flags = app_entry->flags;
      goto end;
    }

    app_entries += FORT_CONF_APP_ENTRY_SIZE(app_entry->path_len);
  } while (--count != 0);

 not_found:
  app_flags.v = 0;

 end:
  return app_flags;
}

static FORT_APP_FLAGS
fort_conf_app_find (const PFORT_CONF conf,
                    const char *path, UINT32 path_len,
                    fort_conf_app_exe_find_func *exe_find_func)
{
  FORT_APP_FLAGS app_flags;

  app_flags = exe_find_func(conf, path, path_len);
  if (app_flags.v != 0)
    goto end;

  app_flags = fort_conf_app_prefix_find(conf, path, path_len);
  if (app_flags.v != 0)
    goto end;

  app_flags = fort_conf_app_wild_find(conf, path);

 end:
  return app_flags;
}

static BOOL
fort_conf_app_blocked (const PFORT_CONF conf, FORT_APP_FLAGS app_flags)
{
  const BOOL app_found = (app_flags.v != 0);

  if (app_found && !app_flags.use_group_perm) {
      return app_flags.blocked;
  } else {
    const UINT32 app_perm_val = app_flags.blocked ? 2 : 1;
    const UINT32 app_perm = app_perm_val << (app_flags.group_index * 2);

    const BOOL block_all = conf->flags.app_block_all;
    const BOOL allow_all = conf->flags.app_allow_all;

    const BOOL app_blocked = block_all ? TRUE : (app_found
      && (app_perm & conf->app_perms_block_mask));
    const BOOL app_allowed = allow_all ? TRUE : (app_found
      && (app_perm & conf->app_perms_allow_mask));

    return block_all ? !app_allowed
      : (allow_all ? app_blocked
      : (app_blocked && !app_allowed));
  }
}

static UINT16
fort_conf_app_period_bits (const PFORT_CONF conf, FORT_TIME time,
                           int *periods_n)
{
  const char *data;
  PFORT_PERIOD app_periods;
  UINT16 period_bits;
  UINT8 count = conf->app_periods_n;
  int n, i;

  if (count == 0)
    return 0;

  data = conf->data;
  app_periods = (const PFORT_PERIOD) (data + conf->app_periods_off);
  period_bits = (UINT16) conf->flags.group_bits;
  n = 0;

  for (i = 0; i < FORT_CONF_GROUP_MAX; ++i) {
    const UINT16 bit = (1 << i);
    const FORT_PERIOD period = *app_periods++;

    if ((period_bits & bit) != 0 && period.v != 0) {
      if (!is_time_in_period(time, period)) {
        period_bits ^= bit;
      }

      ++n;

      if (--count == 0)
        break;
    }
  }

  if (periods_n != NULL) {
    *periods_n = n;
  }

  return period_bits;
}

static void
fort_conf_app_perms_mask_init (PFORT_CONF conf, UINT32 group_bits)
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

  conf->app_perms_block_mask = (perms_mask & 0xAAAAAAAA);
  conf->app_perms_allow_mask = (perms_mask & 0x55555555);
}
