-- WIPF Provider (Un)Registration script

local wipf = require"wipflua"
local sys = require"sys"


local persist, boot

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
  local _, err = wipf.prov_register(persist, boot)
  if err then
    sys.stderr:write("Error: ", sys.strerror(err))
    sys.exit(false)
  end
end

