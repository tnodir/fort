-- WIPF Configuration Model

local sys = require"sys"

local wipf = require"wipflua"
local i18n = require"wipf.util.i18n"
local util_conf = require"wipf.util.conf"


local model_conf = {}

local profile_dir = ""
local conf_filename = "wipf.cfg"


function model_conf.set_profile_dir(dir)
  if profile_dir == dir then return end

  if not dir:find("\\", -1, true) then
    dir = dir .. "\\"
  end

  profile_dir = dir
end

function model_conf.get_profile_dir()
  return profile_dir
end


return model_conf
