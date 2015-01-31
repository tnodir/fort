-- WIPF Configuration Utilities

local bit = require"bit"

local wipf = require"wipflua"
local i18n = require"wipf.util.i18n"
local util_fs = require"wipf.util.fs"
local util_ip = require"wipf.util.ip"


local util_conf = {
  APP_GROUP_MAX = 10,
  APP_GROUP_NAME_MAX = 128
}


local function parse_app(line)
  local path = string.gsub(line, "(\"?%s*)$", "")
  if path == "" then
    return
  end

  path = path:lower()

  local partial

  if path == "system" then
    path, partial = "System", false
  else
    partial = (path:sub(-1) == '*')
    if partial then
      path = path:sub(1, #path - 1)
    end
    path = util_fs.path_to_dospath(path)
  end

  return path, partial
end

local function parse_apps(text, blocked, apps_map, group_index)
  for line in string.gmatch(text, "%s*\"?([^\n]+)") do
    local app, partial = parse_app(line)

    if app then
      local app_3bits = apps_map[app] or 0
      local val_3bit = bit.bor(blocked and 2 or 1, partial and 4 or 0)
      local app_3bit = bit.lshift(val_3bit, group_index*3)
      apps_map[app] = bit.bor(app_3bits, app_3bit)
    end
  end
end

-- Convert app. group objects to plain table
local function app_groups_to_plain(app_groups)
  local group_bits = 0
  local groups, groups_count = {}, app_groups.n
  local apps_map = {}

  if groups_count > util_conf.APP_GROUP_MAX then
    return nil, i18n.tr_fmt('err_conf_group_max', util_conf.APP_GROUP_MAX)
  end

  for i = 1, groups_count do
    local app_group = app_groups[i]
    local group_index = i - 1

    if app_group:get_enabled() then
      local group_bit = bit.lshift(1, group_index)
      group_bits = bit.bor(group_bits, group_bit)
    end

    local name = app_group:get_name()
    if #name > util_conf.APP_GROUP_NAME_MAX then
      return nil, i18n.tr_fmt('err_conf_group_name_max', util_conf.APP_GROUP_NAME_MAX)
    end

    groups[i] = name

    parse_apps(app_group:get_block(), true, apps_map, group_index)
    parse_apps(app_group:get_allow(), false, apps_map, group_index)
  end

  -- fill "apps" array
  local apps, apps_count = {}, 0
  for app in pairs(apps_map) do
    apps_count = apps_count + 1
    apps[apps_count] = app
  end

  table.sort(apps)

  -- fill "apps_3bit" array
  local app_3bits = {}
  for i = 1, apps_count do
    local app = apps[i]
    app_3bits[i] = apps_map[app]
  end

  groups.n = groups_count
  app_3bits.n, apps.n = apps_count, apps_count

  return group_bits, groups, app_3bits, apps
end

-- Calculate total length of strings in table
local function get_strings_len(t, n)
  local len = 0
  for i = 1, n do
    len = len + #t[i]
  end
  return len
end


-- Configuration objects meta-table
local conf_meta = {}

conf_meta.__index = conf_meta

function conf_meta:set_ip_include_all(bool)
  self.ip_include_all = bool
end
function conf_meta:get_ip_include_all()
  return self.ip_include_all
end

function conf_meta:set_ip_exclude_all(bool)
  self.ip_exclude_all = bool
end
function conf_meta:get_ip_exclude_all()
  return self.ip_exclude_all
end

function conf_meta:set_app_log_blocked(bool)
  self.app_log_blocked = bool
end
function conf_meta:get_app_log_blocked()
  return self.app_log_blocked
end

function conf_meta:set_app_block_all(bool)
  self.app_block_all = bool
end
function conf_meta:get_app_block_all()
  return self.app_block_all
end

function conf_meta:set_app_allow_all(bool)
  self.app_allow_all = bool
end
function conf_meta:get_app_allow_all()
  return self.app_allow_all
end

function conf_meta:set_ip_include(str)
  self.ip_include = str
end
function conf_meta:get_ip_include()
  return self.ip_include
end

function conf_meta:set_ip_exclude(str)
  self.ip_exclude = str
end
function conf_meta:get_ip_exclude()
  return self.ip_exclude
end

function conf_meta:add_app_group(app_group)
  local app_groups = self.app_groups
  table.insert(app_groups, app_group)
  app_groups.n = app_groups.n + 1
end

function conf_meta:remove_app_group(index)
  local app_groups = self.app_groups
  table.remove(app_groups, index)
  app_groups.n = app_groups.n - 1
end

-- Write conf. object to buffer
function conf_meta:write(buf)

  local iprange_from_inc, iprange_to_inc =
      util_ip.ip4range_to_numbers(self.ip_include)
  if not iprange_from_inc then
    return nil, i18n.tr_fmt('err_conf_iprange_inc', iprange_to_inc)
  end

  local iprange_from_exc, iprange_to_exc =
      util_ip.ip4range_to_numbers(self.ip_exclude)
  if not iprange_from_exc then
    return nil, i18n.tr_fmt('err_conf_iprange_exc', iprange_to_exc)
  end

  local group_bits, groups, app_3bits, apps =
      app_groups_to_plain(self.app_groups)
  if not group_bits then
    return nil, groups
  end

  -- calculate maximum required buffer size
  local buf_size = wipf.conf_buffer_size(
      iprange_from_inc.n, iprange_from_exc.n,
      groups.n, get_strings_len(groups, groups.n),
      apps.n, get_strings_len(apps, apps.n))
  if not (buf_size and buf:reserve(buf_size)) then
    return nil, i18n.tr('err_conf_size')
  end

  return true
end

-- New conf. object
function util_conf.new_conf()
  return setmetatable({
    ip_include_all = false,
    ip_exclude_all = false,

    app_log_blocked = true,
    app_block_all = true,
    app_allow_all = false,

    ip_include = "",
    ip_exclude = "",

    app_groups = {n = 0}
  }, conf_meta)
end


-- Application group's meta-table
local app_group_meta = {}

app_group_meta.__index = app_group_meta

function app_group_meta:set_name(str)
  self.name = str
end
function app_group_meta:get_name()
  return self.name
end

function app_group_meta:set_enabled(bool)
  self.enabled = bool
end
function app_group_meta:get_enabled()
  return self.enabled
end

function app_group_meta:set_block(str)
  self.block = str
end
function app_group_meta:get_block()
  return self.block
end

function app_group_meta:set_allow(str)
  self.allow = str
end
function app_group_meta:get_allow()
  return self.allow
end

-- New app. group object
function util_conf.new_app_group()
  return setmetatable({
    name = "",
    enabled = true,
    block = "",
    allow = ""
  }, app_group_meta)
end


return util_conf
