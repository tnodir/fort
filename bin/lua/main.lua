-- WIPF GUI

local iup = require"iuplua"
local sys = require"sys"

local i18n = require"wipf.util.i18n"
local model_conf = require"wipf.model.conf"
local ui_window = require"wipf.ui.window"


local window_title = "Windows IP Filter"


local function exit()
  os.exit(false)
end

local function usage()
  print[[
Usage: luajit.exe lua\main.lua <arguments>
Argumets:
	lang=<code> ..... Language code (see "lua\wipf\lang\" folder)
	profile=<path> .. Path to folder with configuration files

Example: luajit.exe lua\main.lua lang=en profile="%LOCALAPPDATA%\wipf"
]]
  exit()
end


-- Initialize IUP
iup.SetGlobal("UTF8MODE", "YES")
iup.SetGlobal("UTF8MODE_FILE", "YES")
iup.SetGlobal("DEFAULTFONTSIZE", 11)


-- Check running instance
iup.SetGlobal("SINGLEINSTANCE", window_title)

if not iup.GetGlobal("SINGLEINSTANCE") then
  print"Error: Already running."
  exit()
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


-- Initialize UI
ui_window.init(window_title)


-- Event Loop
do
  local function on_winmsg(evq)
    if iup.LoopStep() == iup.CLOSE then
      evq:stop()
    end
    iup.Flush()
  end

  local evq = sys.event_queue()
  evq:add_winmsg(nil, on_winmsg)

  evq:loop()
end

