-- WIPF IP Utilities

local sys = require"sys"
local sock = require"sys.sock"


-- Convert IPv4 ranges in text to 'from_ip4' & 'to_ip4' arrays with numbers
local function ip4range_to_numbers(text)
  local from, to = {}, {}

  return from, to
end


return {
  ip4range_to_numbers	= ip4range_to_numbers,
}
