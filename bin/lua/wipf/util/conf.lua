-- WIPF Configuration Utilities

local bit = require"bit"

local wipf = require"wipflua"
local i18n = require"wipf.util.i18n"
local util_fs = require"wipf.util.fs"
local util_ip = require"wipf.util.ip"


local util_conf = {
  APP_GROUP_MAX = 16,
  APP_GROUP_NAME_MAX = 128,
  APP_PATH_MAX = 1024
}


local function parse_app(line)
  local path = string.gsub(line, "(\"?%s*)$", "")
  if path == "" then
    return
  end

  path = path:lower()

  if path == "system" then
    path = "System"
  else
    path = util_fs.path_to_dospath(path)
  end

  return path
end

local function parse_apps(text, blocked, apps_map, group_offset)
  for line in string.gmatch(text, "%s*\"?([^\n]+)") do
    local app = parse_app(line)

    if app then
      local app_perms = apps_map[app] or 0
      local val_perm = blocked and 2 or 1
      local app_perm = bit.lshift(val_perm, group_offset * 2)
      apps_map[app] = bit.bor(app_perms, app_perm)
    end
  end
end

-- Convert app. group objects to plain tables
local function app_groups_to_plain(app_groups)
  local group_bits = 0
  local groups, groups_count = {}, app_groups.n
  local apps_map = {}

  if groups_count > util_conf.APP_GROUP_MAX then
    return nil, i18n.tr_fmt('err_conf_group_max', util_conf.APP_GROUP_MAX)
  end

  for i = 1, groups_count do
    local app_group = app_groups[i]
    local group_offset = i - 1

    local name = app_group:get_name()
    if #name > util_conf.APP_GROUP_NAME_MAX then
      return nil, i18n.tr_fmt('err_conf_group_name_max', util_conf.APP_GROUP_NAME_MAX)
    end

    groups[i] = name

    if app_group:get_enabled() then
      local group_bit = bit.lshift(1, group_offset)
      group_bits = bit.bor(group_bits, group_bit)
    end

    parse_apps(app_group:get_block(), true, apps_map, group_offset)
    parse_apps(app_group:get_allow(), false, apps_map, group_offset)
  end

  -- fill "apps" array
  local apps, apps_count = {}, 0
  for app in pairs(apps_map) do
    if #app > util_conf.APP_PATH_MAX then
      return nil, i18n.tr_fmt('err_conf_app_path_max', util_conf.APP_PATH_MAX)
    end

    apps_count = apps_count + 1
    apps[apps_count] = app
  end

  table.sort(apps)

  -- fill "apps_perms" array
  local apps_perms = {}
  for i = 1, apps_count do
    local app = apps[i]
    apps_perms[i] = apps_map[app]
  end

  groups.n = groups_count
  apps_perms.n, apps.n = apps_count, apps_count

  return group_bits, groups, apps_perms, apps
end

-- Create app. group objects from plain tables
local function app_groups_from_plain(group_bits, groups, apps_perms, apps)
  local app_groups = {}
  local groups_count, apps_count = groups.n, apps.n

  for i = 1, groups_count do
    local app_group = util_conf.new_app_group()
    local group_offset = i - 1

    app_group:set_name(groups[i])

    do
      local group_bit = bit.lshift(1, group_offset)
      local val = bit.band(group_bits, group_bit)
      app_group:set_enabled(val ~= 0)
    end

    -- fill 'block' & 'allow' apps
    local block, allow = "", ""

    for app_index = 1, apps_count do
      local app_perms = apps_perms[app_index]
      local val_perm = bit.rshift(app_perms, group_offset * 2)
      local allowed = bit.band(val_perm, 1) ~= 0
      local blocked = bit.band(val_perm, 2) ~= 0

      if allowed or blocked then
        local app = apps[app_index]

        app = util_fs.dospath_to_path(app)

        if blocked then
          block = block .. app .. "\n"
        end
        if allowed then
          allow = allow .. app .. "\n"
        end
      end
    end

    app_group:set_block(block)
    app_group:set_allow(allow)

    app_groups[i] = app_group
  end

  app_groups.n = groups_count

  return app_groups
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

function conf_meta:get_app_groups()
  return self.app_groups
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

  local group_bits, groups, apps_perms, apps =
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

  if self.app_block_all and self.app_allow_all then
    return nil, i18n.tr('err_conf_app_block_allow')
  end

  local conf_size = wipf.conf_write(buf:getptr(),
      self.ip_include_all, self.ip_exclude_all,
      self.app_log_blocked,
      self.app_block_all, self.app_allow_all,
      iprange_from_inc.n, iprange_from_inc, iprange_to_inc,
      iprange_from_exc.n, iprange_from_exc, iprange_to_exc,
      apps.n, apps_perms, apps,
      group_bits, groups.n, groups)

  buf:seek(conf_size)

  return conf_size
end

-- Read conf. object from buffer
function conf_meta:read(buf)

  local ip_include_all, ip_exclude_all,
      app_log_blocked, app_block_all, app_allow_all,
      iprange_from_inc, iprange_to_inc,
      iprange_from_exc, iprange_to_exc,
      apps_perms, apps, group_bits, groups = wipf.conf_read(buf:getptr())

  self.ip_include_all = ip_include_all
  self.ip_exclude_all = ip_exclude_all
  self.app_log_blocked = app_log_blocked
  self.app_block_all = app_block_all
  self.app_allow_all = app_allow_all

  self.ip_include = util_ip.ip4range_from_numbers(
      iprange_from_inc, iprange_to_inc)

  self.ip_exclude = util_ip.ip4range_from_numbers(
      iprange_from_exc, iprange_to_exc)

  self.app_groups = app_groups_from_plain(
      group_bits, groups, apps_perms, apps)
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
