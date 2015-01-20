-- WIPF IP Utilities

local bit = require"bit"

local sys = require"sys"
local sock = require"sys.sock"


-- Convert IPv4 ranges in text to 'from_ip4' & 'to_ip4' arrays with numbers
local ip4range_to_numbers
do
  local function parse_address_mask(line)
    local from, sep, mask = string.match(line, "([%d%.]+)%s*([/%-])%s*(%S+)")
    if not from then
      return
    end

    local from_ip = sock.inet_pton(from, true)
    if not from_ip then
      return
    end

    local to_ip
    if sep == '-' then  -- e.g. "127.0.0.0-127.255.255.255"
      to_ip = sock.inet_pton(mask, true)
      if not to_ip then
        return
      end
    elseif sep == '/' then  -- e.g. "127.0.0.0/24"
      local nbits = tonumber(mask)
      if nbits > 32 or nbits < 0 then
        return
      elseif nbits == 32 then
        to_ip = 0xFFFFFFFF
      else
        to_ip = bit.bor(from_ip, bit.lshift(1, nbits) - 1)
      end
    end

    return from_ip, to_ip
  end

  ip4range_to_numbers = function (text)
    local iprange_from, iprange_to = {}, {}
    local line_no, index = 0, 0

    for line in string.gmatch(text, "%s*([^\n]+)") do
      local from, to = parse_address_mask(line)

      line_no = line_no + 1

      if from then
        index = index + 1
        iprange_from[index], iprange_to[index] = from, to
      elseif string.find(line, "%S") then
        return nil, line_no
      end
    end

    iprange_from.n, iprange_to.n = index, index

    return iprange_from, iprange_to
  end
end


return {
  ip4range_to_numbers	= ip4range_to_numbers,
}
