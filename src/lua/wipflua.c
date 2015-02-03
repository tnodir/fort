/* Windows IP Filter: Lua interface */

#define _WIN32_WINNT	0x0600

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winioctl.h>
#include <fwpmu.h>

#define LUA_LIB

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../common.h"
#include "../wipfconf.h"

#include "../wipfconf.c"
#include "../wipflog.c"
#include "../wipfprov.c"


/*
 * Returns: string
 */
static int
wipf_lua_device_name (lua_State *L)
{
  lua_pushliteral(L, WIPF_DEVICE_NAME);
  return 1;
}

/*
 * Returns: number
 */
static int
wipf_lua_ioctl_getlog (lua_State *L)
{
  lua_pushinteger(L, WIPF_IOCTL_GETLOG);
  return 1;
}

/*
 * Returns: number
 */
static int
wipf_lua_ioctl_setconf (lua_State *L)
{
  lua_pushinteger(L, WIPF_IOCTL_SETCONF);
  return 1;
}

/*
 * Returns: number
 */
static int
wipf_lua_buffer_size (lua_State *L)
{
  lua_pushinteger(L, WIPF_BUFFER_SIZE);
  return 1;
}

/*
 * Arguments: output (ludata), remote_ip (number),
 *	pid (number), path (string)
 * Returns: length (number)
 */
static int
wipf_lua_log_write (lua_State *L)
{
  char *out = lua_touserdata(L, 1);
  const UINT32 remote_ip = lua_tointeger(L, 2);
  const UINT32 pid = lua_tointeger(L, 3);
  UINT32 path_len;
  const char *path = lua_tolstring(L, 4, &path_len);

  if (!out) return 0;

  wipf_log_write(out, remote_ip, pid, path_len, path);

  lua_pushinteger(L, WIPF_LOG_SIZE(path_len));
  return 1;
}

/*
 * Arguments: input (ludata), [offset (number)]
 * Returns: length (number), remote_ip (number),
 *	pid (number), [path (string)]
 */
static int
wipf_lua_log_read (lua_State *L)
{
  char *in = lua_touserdata(L, 1);
  const int off = lua_tointeger(L, 2);
  UINT32 remote_ip, pid;
  UINT32 path_len;
  const char *path;

  if (!in) return 0;

  wipf_log_read(in + off, &remote_ip, &pid, &path_len, &path);

  lua_pushinteger(L, WIPF_LOG_SIZE(path_len));
  lua_pushnumber(L, remote_ip);
  lua_pushinteger(L, pid);

  if (path_len) {
    char buf[WIPF_LOG_PATH_MAX];
    const int n = WideCharToMultiByte(
        CP_UTF8, 0, (LPCWCH) path, path_len / sizeof(WCHAR),
        buf, WIPF_LOG_PATH_MAX, NULL, 0);

    lua_pushlstring(L, buf, n);
    return 4;
  }
  return 3;
}


static void
wipf_lua_conf_write_strtable (lua_State *L, int index, int count, char **data)
{
  UINT32 *offp = (UINT32 *) *data;
  const UINT32 head_size = (count + 1) * sizeof(UINT32);
  char *p = *data + head_size;
  UINT32 off;
  int i;

  luaL_checktype(L, index, LUA_TTABLE);

  off = 0;
  *offp++ = 0;

  for (i = 1; i <= count; ++i) {
    const char *path;
    size_t len;

    lua_rawgeti(L, index, i);
    path = lua_tolstring(L, -1, &len);
    lua_pop(L, 1);

    len = MultiByteToWideChar(
        CP_UTF8, 0, path, len,
        (LPWCH) p, WIPF_CONF_APP_PATH_MAX / sizeof(WCHAR));
    len *= sizeof(WCHAR);

    off += (UINT32) len;
    *offp++ = off;
    p += len;
  }

  *data += head_size + (off + (WIPF_CONF_STR_ALIGN-1)) & ~(WIPF_CONF_STR_ALIGN-1);
}

static void
wipf_lua_conf_write_numtable (lua_State *L, int index, int count, char **data)
{
  UINT32 *p = (UINT32 *) *data;
  int i;

  luaL_checktype(L, index, LUA_TTABLE);

  for (i = 1; i <= count; ++i) {
    lua_rawgeti(L, index, i);
    *p++ = (UINT32) lua_tonumber(L, -1);
    lua_pop(L, 1);
  }

  *data = (char *) p;
}

/*
 * Arguments: ip_include_n (number), ip_exclude_n (number)
 *	groups_n (number), groups_len (number),
 *	apps_n (number), apps_len (number)
 * Returns: length (number)
 */
static int
wipf_lua_conf_buffer_size (lua_State *L)
{
  const int ip_include_n = lua_tointeger(L, 1);
  const int ip_exclude_n = lua_tointeger(L, 2);
  const int groups_n = lua_tointeger(L, 3);
  const int groups_len = lua_tointeger(L, 4) * sizeof(WCHAR);
  const int apps_n = lua_tointeger(L, 5);
  const int apps_len = lua_tointeger(L, 6) * sizeof(WCHAR);

  if (ip_include_n > WIPF_CONF_IP_MAX
      || ip_exclude_n > WIPF_CONF_IP_MAX
      || groups_len > WIPF_CONF_GROUPS_LEN_MAX
      || apps_len > WIPF_CONF_APPS_LEN_MAX)
    return 0;

  lua_pushinteger(L, WIPF_CONF_SIZE_MIN
      + (ip_include_n + ip_exclude_n) * 2 * sizeof(UINT32)
      + (groups_n + apps_n) * sizeof(UINT32)
      + groups_len + apps_len
      + apps_n * sizeof(UINT32));
  return 1;
}

/*
 * Arguments: output (ludata),
 *	ip_include_all (boolean),
 *	ip_exclude_all (boolean),
 *	app_log_blocked (boolean),
 *	app_block_all (boolean),
 *	app_allow_all (boolean)
 *	ip_include_n (number),
 *	ip_from_include (table: 1..n => number),
 *	ip_to_include (table: 1..n => number),
 *	ip_exclude_n (number),
 *	ip_from_exclude (table: 1..n => number),
 *	ip_to_exclude (table: 1..n => number),
 *	apps_n (number),
 *	apps_perms (table: 1..n => number),
 *	apps (table: 1..n => string),
 *	group_bits (number),
 *	groups_n (number),
 *	groups (table: 1..n => string)
 * Returns: length (number)
 */
static int
wipf_lua_conf_write (lua_State *L)
{
  PWIPF_CONF conf = lua_touserdata(L, 1);
  const BOOL ip_include_all = lua_toboolean(L, 2);
  const BOOL ip_exclude_all = lua_toboolean(L, 3);
  const BOOL app_log_blocked = lua_toboolean(L, 4);
  const BOOL app_block_all = lua_toboolean(L, 5);
  const BOOL app_allow_all = lua_toboolean(L, 6);
  const UINT32 ip_include_n = lua_tointeger(L, 7);
  const UINT32 ip_exclude_n = lua_tointeger(L, 10);
  const UINT32 apps_n = lua_tointeger(L, 13);
  const UINT32 group_bits = lua_tointeger(L, 16);
  const UINT32 groups_n = lua_tointeger(L, 17);
  UINT32 ip_from_include_off, ip_to_include_off;
  UINT32 ip_from_exclude_off, ip_to_exclude_off;
  UINT32 groups_off, apps_off, apps_perms_off;
  UINT32 conf_size;
  char *data;

  if (!conf) return 0;

  data = (char *) &conf->data;

#define data_offset	(data - (char *) &conf->data)
  ip_from_include_off = data_offset;
  wipf_lua_conf_write_numtable(L, 8, ip_include_n, &data);  /* ip_from_include */

  ip_to_include_off = data_offset;
  wipf_lua_conf_write_numtable(L, 9, ip_include_n, &data);  /* ip_to_include */

  ip_from_exclude_off = data_offset;
  wipf_lua_conf_write_numtable(L, 11, ip_exclude_n, &data);  /* ip_from_exclude */

  ip_to_exclude_off = data_offset;
  wipf_lua_conf_write_numtable(L, 12, ip_exclude_n, &data);  /* ip_to_exclude */

  apps_perms_off = data_offset;
  wipf_lua_conf_write_numtable(L, 14, apps_n, &data);  /* apps_perms */

  apps_off = data_offset;
  wipf_lua_conf_write_strtable(L, 15, apps_n, &data);  /* apps */

  groups_off = data_offset;
  wipf_lua_conf_write_strtable(L, 18, groups_n, &data);  /* groups */

  conf_size = WIPF_CONF_SIZE_MIN + data_offset;
#undef data_offset

  conf->ip_include_all = ip_include_all;
  conf->ip_exclude_all = ip_exclude_all;
  conf->app_log_blocked = app_log_blocked;
  conf->app_block_all = app_block_all;
  conf->app_allow_all = app_allow_all;

  conf->ip_include_n = ip_include_n;
  conf->ip_exclude_n = ip_exclude_n;

  conf->apps_n = apps_n;
  conf->groups_n = groups_n;

  wipf_conf_group_bits_set(conf, group_bits);

  conf->ip_from_include_off = ip_from_include_off;
  conf->ip_to_include_off = ip_to_include_off;

  conf->ip_from_exclude_off = ip_from_exclude_off;
  conf->ip_to_exclude_off = ip_to_exclude_off;

  conf->apps_perms_off = apps_perms_off;
  conf->apps_off = apps_off;
  conf->groups_off = groups_off;

  lua_pushinteger(L, conf_size);
  return 1;
}


static void
wipf_lua_conf_read_strtable (lua_State *L, int count, const char *data)
{
  const UINT32 *offp = (const UINT32 *) data;
  const char *p = data + (count + 1) * sizeof(UINT32);
  int i;

  lua_createtable(L, count, 1);

  for (i = 1; i <= count; ++i) {
    char buf[WIPF_CONF_APP_PATH_MAX];
    const int off = offp[i - 1];
    const char *path = p + off;
    int len = offp[i] - off;

    len = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH) path, len / sizeof(WCHAR),
                              buf, WIPF_CONF_APP_PATH_MAX, NULL, 0);

    lua_pushlstring(L, buf, len);
    lua_rawseti(L, -2, i);
  }

  lua_pushinteger(L, count);
  lua_setfield(L, -2, "n");
}

static void
wipf_lua_conf_read_numtable (lua_State *L, int count, const char *data)
{
  const UINT32 *p = (const UINT32 *) data;
  int i;

  lua_createtable(L, count, 1);

  for (i = 1; i <= count; ++i) {
    lua_pushnumber(L, *p++);
    lua_rawseti(L, -2, i);
  }

  lua_pushinteger(L, count);
  lua_setfield(L, -2, "n");
}

/*
 * Arguments: input (ludata)
 * Returns:
 *	ip_include_all (boolean),
 *	ip_exclude_all (boolean),
 *	app_log_blocked (boolean),
 *	app_block_all (boolean),
 *	app_allow_all (boolean)
 *	ip_from_include (table: 1..n => number),
 *	ip_to_include (table: 1..n => number),
 *	ip_from_exclude (table: 1..n => number),
 *	ip_to_exclude (table: 1..n => number),
 *	apps_perms (table: 1..n => number),
 *	apps (table: 1..n => string),
 *	group_bits (number),
 *	groups (table: 1..n => string)
 */
static int
wipf_lua_conf_read (lua_State *L)
{
  const PWIPF_CONF conf = lua_touserdata(L, 1);
  const char *data = (const char *) &conf->data;

  if (!conf) return 0;

  lua_pushboolean(L, conf->ip_include_all);
  lua_pushboolean(L, conf->ip_exclude_all);
  lua_pushboolean(L, conf->app_log_blocked);
  lua_pushboolean(L, conf->app_block_all);
  lua_pushboolean(L, conf->app_allow_all);

  wipf_lua_conf_read_numtable(L, conf->ip_include_n,
      data + conf->ip_from_include_off);  /* ip_from_include */

  wipf_lua_conf_read_numtable(L, conf->ip_include_n,
      data + conf->ip_to_include_off);  /* ip_to_include */

  wipf_lua_conf_read_numtable(L, conf->ip_exclude_n,
      data + conf->ip_from_exclude_off);  /* ip_from_exclude */

  wipf_lua_conf_read_numtable(L, conf->ip_exclude_n,
      data + conf->ip_to_exclude_off);  /* ip_to_exclude */

  wipf_lua_conf_read_numtable(L, conf->apps_n,
      data + conf->apps_perms_off);  /* apps_perms */

  wipf_lua_conf_read_strtable(L, conf->apps_n,
      data + conf->apps_off);  /* apps */

  lua_pushinteger(L, conf->group_bits);

  wipf_lua_conf_read_strtable(L, conf->groups_n,
      data + conf->groups_off);  /* groups */

  return 13;
}

/*
 * Arguments: input (ludata), ip (number), included/excluded (boolean)
 * Returns: boolean
 */
static int
wipf_lua_conf_ip_inrange (lua_State *L)
{
  const PWIPF_CONF conf = lua_touserdata(L, 1);
  const UINT32 ip = (UINT32) lua_tonumber(L, 2);
  const BOOL included = lua_toboolean(L, 3);
  const char *data = (const char *) &conf->data;

  const UINT32 count = included ? conf->ip_include_n : conf->ip_exclude_n;
  const UINT32 from_off = included ? conf->ip_from_include_off : conf->ip_from_exclude_off;
  const UINT32 to_off = included ? conf->ip_to_include_off : conf->ip_to_exclude_off;

  const BOOL res = wipf_conf_ip_inrange(ip, count,
      (const UINT32 *) (data + from_off),
      (const UINT32 *) (data + to_off));

  lua_pushboolean(L, res);
  return 1;
}

/*
 * Arguments: input (ludata), dos_path (string)
 * Returns: blocked (boolean), notify (boolean)
 */
static int
wipf_lua_conf_app_blocked (lua_State *L)
{
  const PWIPF_CONF conf = lua_touserdata(L, 1);
  size_t len;
  const char *path = luaL_checklstring(L, 2, &len);
  WCHAR buf[WIPF_CONF_APP_PATH_MAX];
  BOOL blocked, notify;

  len = MultiByteToWideChar(CP_UTF8, 0, path, len,
                            buf, WIPF_CONF_APP_PATH_MAX);
  len *= sizeof(WCHAR);

  blocked = wipf_conf_app_blocked(conf, len, (const char *) buf, &notify);

  lua_pushboolean(L, blocked);
  lua_pushboolean(L, notify);
  return 2;
}

/*
 * Arguments: boot (boolean)
 * Returns: boolean | nil, err_code
 */
static int
wipf_lua_prov_register (lua_State *L)
{
  const BOOL boot = lua_toboolean(L, 1);
  const DWORD status = wipf_prov_register(TRUE, boot, NULL);

  if (!status) {
    lua_pushboolean(L, 1);
    return 1;
  }

  lua_pushnil(L);
  lua_pushinteger(L, status);
  return 2;
}

static int
wipf_lua_prov_unregister (lua_State *L)
{
  (void) L;

  wipf_prov_unregister();
  return 0;
}


static luaL_Reg wipf_lib[] = {
  {"device_name",	wipf_lua_device_name},
  {"ioctl_getlog",	wipf_lua_ioctl_getlog},
  {"ioctl_setconf",	wipf_lua_ioctl_setconf},
  {"buffer_size",	wipf_lua_buffer_size},
  {"log_write",		wipf_lua_log_write},
  {"log_read",		wipf_lua_log_read},
  {"conf_buffer_size",	wipf_lua_conf_buffer_size},
  {"conf_write",	wipf_lua_conf_write},
  {"conf_read",		wipf_lua_conf_read},
  {"conf_ip_inrange",	wipf_lua_conf_ip_inrange},
  {"conf_app_blocked",	wipf_lua_conf_app_blocked},
  {"prov_register",	wipf_lua_prov_register},
  {"prov_unregister",	wipf_lua_prov_unregister},
  {NULL, NULL}
};


LUALIB_API int
luaopen_wipflua (lua_State *L)
{
  luaL_register(L, "wipflua", wipf_lib);
  return 1;
}
