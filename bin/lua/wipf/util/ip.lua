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
      if not to_ip or from_ip > to_ip then
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

  -- sort and try to merge ranges
  local function iprange_map_merge(map)
    -- fill temporary "from" range
    local tmp_from, tmp_count = {}, 0
    for from in pairs(map) do
      tmp_count = tmp_count + 1
      tmp_from[tmp_count] = from
    end

    table.sort(tmp_from)

    -- try to merge ranges
    local prev_from, prev_to
    for i = 1, tmp_count do
      local from = tmp_from[i]
      local to = map[from]
      if prev_from and from <= prev_to then -- collides with previous?
        if to > prev_to then
          map[prev_from], prev_to = to, to
        end
        tmp_from[i] = nil
      else
        prev_from, prev_to = from, to
      end
    end

    -- fill "from" & "to" ranges
    local iprange_from, iprange_to, count = {}, {}, 0
    for i = 1, tmp_count do
      local from = tmp_from[i]
      if from then
        count = count + 1
        iprange_from[count] = from
        iprange_to[count] = map[from]
      end
    end

    iprange_from.n, iprange_to.n = count, count

    return iprange_from, iprange_to
  end

  ip4range_to_numbers = function (text)
    local iprange_map = {}
    local line_no = 0

    for line in string.gmatch(text, "%s*([^\n]+)") do
      local from, to = parse_address_mask(line)

      line_no = line_no + 1

      if from then
        iprange_map[from] = to
      elseif string.find(line, "%S") then
        return nil, line_no
      end
    end

    return iprange_map_merge(iprange_map)
  end
end


return {
  ip4range_to_numbers	= ip4range_to_numbers,
}
