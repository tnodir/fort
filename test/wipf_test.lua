-- WIPF Log Tests

local sys = require"sys"

local wipf = require"wipflua"
local wipf_fs = require"wipf/util/fs"
local wipf_ip = require"wipf/util/ip"

local mem, win32 = sys.mem, sys.win32


print"-- Log Read/Write"
do
  local path = [[C:\test]]
  assert(#path == 7)

  local outlen = 12 + #path + 1
  local bufsize = outlen + 4
  local buf = assert(mem.pointer(bufsize))

  buf:memset(0, bufsize)
  buf[outlen] = 127
  assert(wipf.log_write(buf:getptr(), 1, 2, path) == outlen)
  assert(buf[outlen] == 127)

  local inlen, ip, pid = wipf.log_read(buf:getptr())
  assert(inlen == outlen and ip == 1 and pid == 2)

  print("OK")
end


print"-- Paths"
do
  local path = [[C:\test]]
  local dos_path = wipf_fs.path_to_dospath(path)
  local win32_path = wipf_fs.dospath_to_path(dos_path)
  assert(path == win32_path)
  print("OK")
end


print"-- Conf Read/Write"
do
  local log_blocked = true
  local ip_include = "*"
  local ip_exclude = [[
    10.0.0.0/24
    127.0.0.0/24
    169.254.0.0/16
    172.16.0.0/20
    192.168.0.0/16
  ]]
  local app_block = [[
    *
    System
  ]]
  local app_permit = [[
    D:\Programs\Skype\Phone\Skype.exe
    D:\Utils\Firefox\Bin\firefox.exe
    D:\Utils\Dev\Git\*
  ]]

  local iprange_from_inc, iprange_to_inc =
      wipf_ip.ip4range_to_numbers(ip_include)
  assert(iprange_from_inc.n == iprange_to_inc.n)

  local iprange_from_exc, iprange_to_exc =
      wipf_ip.ip4range_to_numbers(ip_exclude)
  assert(iprange_from_exc.n == iprange_to_exc.n)

  print("OK")
end


