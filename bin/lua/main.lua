-- WIPF GUI

local iup = require"iuplua"
local sys = require"sys"

local i18n = require"wipf.util.i18n"
local model_conf = require"wipf.model.conf"


local function usage()
  print[[
Usage: luajit.exe lua\main.lua <arguments>
Argumets:
	lang ..... Language code (see "lua\wipf\lang\" folder)
	profile .. Path to folder with configuration files

Example: luajit.exe lua\main.lua lang=en profile="%LOCALAPPDATA%\wipf"
]]
  sys.exit(false)
end


-- Process arguments
for _, a in ipairs(arg) do
  local k, v = string.match(a, "(%w+)=(.+)")
  if k == "lang" then
    i18n.set_current_lang(v)
  elseif k == "profile" then
    model_conf.set_profile_dir(v)
  elseif a == "help" then
    usage()
  end
end


--iup.MainLoop()
