-- WIPF Provider (Un)Registration script

local wipf = require"wipflua"
local sys = require"sys"


local persist, boot

if #arg == 0 then
  print[[
Usage: luajit.exe scripts/provider.lua <arguments>
Argumets:
	persist ... Register provider, otherwise unregister
	boot ...... Block access to network when WIPF is not running
]]
end

-- Process arguments
for _, v in ipairs(arg) do
  if v == "persist" then
    persist = true
  elseif v == "boot" then
    boot = true
  end
end

wipf.prov_unregister()

if persist then
  local _, err = wipf.prov_register(boot)
  if err then
    sys.stderr:write("Error: ", sys.strerror(err))
    sys.exit(false)
  end
end

