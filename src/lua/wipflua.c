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

#include "../wipflog.c"


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
  lua_pushinteger(L, remote_ip);
  lua_pushinteger(L, pid);

  if (path_len) {
    char buf[WIPF_LOG_PATH_MAX];
    const int n = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH) path, path_len/2,
                                      buf, WIPF_LOG_PATH_MAX, NULL, 0);

    lua_pushlstring(L, buf, n);
    return 4;
  }
  return 3;
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
  const int groups_len = lua_tointeger(L, 4) * 2;
  const int apps_n = lua_tointeger(L, 5);
  const int apps_len = lua_tointeger(L, 6) * 2;

  if (ip_include_n > WIPF_CONF_IP_MAX
      || ip_exclude_n > WIPF_CONF_IP_MAX
      || groups_len > WIPF_CONF_GROUPS_LEN_MAX
      || apps_len > WIPF_CONF_APPS_LEN_MAX)
    return 0;

  lua_pushinteger(L, WIPF_CONF_SIZE_MIN
      + (ip_include_n + ip_exclude_n) * 2 * sizeof(UINT32)
      + (groups_n + apps_n) * sizeof(UINT16)
      + groups_len + apps_len
      + apps_n * sizeof(UINT32));
  return 1;
}


static luaL_Reg wipf_lib[] = {
  {"device_name",	wipf_lua_device_name},
  {"ioctl_getlog",	wipf_lua_ioctl_getlog},
  {"ioctl_setconf",	wipf_lua_ioctl_setconf},
  {"buffer_size",	wipf_lua_buffer_size},
  {"log_write",		wipf_lua_log_write},
  {"log_read",		wipf_lua_log_read},
  {"conf_buffer_size",	wipf_lua_conf_buffer_size},
  {NULL, NULL}
};


LUALIB_API int
luaopen_wipflua (lua_State *L)
{
  luaL_register(L, "wipflua", wipf_lib);
  return 1;
}
