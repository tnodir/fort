-- WIPF Configuration Utilities

local wipf = require"wipflua"
local util_ip = require"wipf/util/ip"


-- Configuration objects meta-table
local conf_meta = {

  set_ip_include_all = function (self, bool)
    self.ip_include_all = bool
  end,
  get_ip_include_all = function (self)
    return self.ip_include_all
  end,

  set_ip_exclude_all = function (self, bool)
    self.ip_exclude_all = bool
  end,
  get_ip_exclude_all = function (self)
    return self.ip_exclude_all
  end,

  set_app_log_blocked = function (self, bool)
    self.app_log_blocked = bool
  end,
  get_app_log_blocked = function (self)
    return self.app_log_blocked
  end,

  set_app_block_all = function (self, bool)
    self.app_block_all = bool
  end,
  get_app_block_all = function (self)
    return self.app_block_all
  end,

  set_app_allow_all = function (self, bool)
    self.app_allow_all = bool
  end,
  get_app_allow_all = function (self)
    return self.app_allow_all
  end,

  set_ip_include = function (self, str)
    self.ip_include = str
  end,
  get_ip_include = function (self)
    return self.ip_include
  end,

  set_ip_exclude = function (self, str)
    self.ip_exclude = str
  end,
  get_ip_exclude = function (self)
    return self.ip_exclude
  end,

  add_app_group = function (self, app_group)
    local app_groups = self.app_groups
    table.insert(app_groups, app_group)
    app_groups.n = app_groups.n + 1
  end,

  remove_app_group = function (self, index)
    local app_groups = self.app_groups
    table.remove(app_groups, index)
    app_groups.n = app_groups.n - 1
  end,

  -- Write conf. object to buffer
  write = function (self, buf)

    local iprange_from_inc, iprange_to_inc =
        util_ip.ip4range_to_numbers(self.ip_include)

    local iprange_from_exc, iprange_to_exc =
        util_ip.ip4range_to_numbers(self.ip_exclude)

    return true
  end,
}

conf_meta.__index = conf_meta

-- New conf. object
local function new_conf()
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
local app_group_meta = {

  set_name = function (self, str)
    self.name = str
  end,
  get_name = function (self)
    return self.name
  end,

  set_enabled = function (self, bool)
    self.enabled = bool
  end,
  get_enabled = function (self)
    return self.enabled
  end,

  set_block = function (self, str)
    self.block = str
  end,
  get_block = function (self)
    return self.block
  end,

  set_allow = function (self, str)
    self.allow = str
  end,
  get_allow = function (self)
    return self.allow
  end,
}

app_group_meta.__index = app_group_meta

-- New app. group object
local function new_app_group()
  return setmetatable({
    name = "",
    enabled = true,
    block = "",
    allow = ""
  }, app_group_meta)
end


return {
  new_conf	= new_conf,
  new_app_group	= new_app_group,
}
