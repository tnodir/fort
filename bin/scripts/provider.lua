-- WIPF Provider (Un)Registration script

local wipf = require"wipflua"
local sys = require"sys"


local register, boot

if #arg == 0 then
  print[[
Usage: luajit.exe scripts/provider.lua <arguments>
Argumets:
	register .... Register provider
	boot ........ Block access to network when WIPF is not running
	unregister .. Unregister provider
]]
end

-- Process arguments
for _, v in ipairs(arg) do
  if v == "register" then
    register = true
  elseif v == "boot" then
    boot = true
  end
end

wipf.prov_unregister()

if register then
  local _, err = wipf.prov_register(boot)
  if err then
    sys.stderr:write("Error: ", sys.strerror(err))
    sys.exit(false)
  end
end

