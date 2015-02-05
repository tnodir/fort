-- WIPF Provider (Un)Registration script

local wipf = require"wipflua"
local sys = require"sys"


local register, boot


local function usage()
  print[[
Usage: luajit.exe scripts\provider.lua <arguments>
Argumets:
	register .... Register provider
	boot ........ Block access to network when WIPF is not running
	unregister .. Unregister provider
]]
  sys.exit(false)
end


-- Process arguments
if #arg == 0 then
  usage()
end

for _, a in ipairs(arg) do
  if a == "register" then
    register = true
  elseif a == "boot" then
    boot = true
  elseif a == "help" then
    usage()
  end
end

-- Unregister
wipf.prov_unregister()

-- Register optionally
if register then
  local _, err = wipf.prov_register(boot)
  if err then
    sys.stderr:write("Error: ", sys.strerror(err))
    sys.exit(false)
  end
end

