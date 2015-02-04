-- WIPF Log Tests

local sys = require"sys"
local sock = require"sys.sock"

local wipf = require"wipflua"
local i18n = require"wipf.util.i18n"
local util_conf = require"wipf.util.conf"
local util_fs = require"wipf.util.fs"
local util_ip = require"wipf.util.ip"

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
  local dos_path = util_fs.path_to_dospath(path)
  local win32_path = util_fs.dospath_to_path(dos_path)
  assert(path == win32_path)
  print("OK")
end


print"-- IPv4 Conversions"
do
  local _, err_line
  _, err_line = util_ip.ip4range_to_numbers[[172.16.0.0/33]]
  assert(err_line == 1)
  _, err_line = util_ip.ip4range_to_numbers[[172.16.0.255/-16]]
  assert(err_line == 1)
  _, err_line = util_ip.ip4range_to_numbers[[10.0.0.32 - 10.0.0.24]]
  assert(err_line == 1)

  local from, to
  from, to = util_ip.ip4range_to_numbers[[
    172.16.0.0/20
    192.168.0.0 - 192.168.255.255
  ]]
  assert(from.n == 2 and to.n == 2)
  assert(from[1] == sock.inet_pton("172.16.0.0", true))
  assert(to[1] == sock.inet_pton("172.31.255.255", true))
  assert(from[2] == sock.inet_pton("192.168.0.0", true))
  assert(to[2] == sock.inet_pton("192.168.255.255", true))

  -- merge ranges
  from, to = util_ip.ip4range_to_numbers[[
    10.0.0.0 - 10.0.0.255
    10.0.0.64 - 10.0.0.128
    10.0.0.128 - 10.0.2.0
  ]]
  assert(from.n == 1 and to.n == 1)
  assert(from[1] == sock.inet_pton("10.0.0.0", true))
  assert(to[1] == sock.inet_pton("10.0.2.0", true))

  print("OK")
end


print"-- Conf Read/Write"
do
  local conf = util_conf.new_conf()

  conf:set_ip_include_all(true)
  conf:set_ip_exclude_all(false)

  conf:set_app_log_blocked(true)
  conf:set_app_block_all(true)
  conf:set_app_allow_all(false)

  conf:set_ip_include""
  conf:set_ip_exclude[[
    10.0.0.0/24
    127.0.0.0/24
    169.254.0.0/16
    172.16.0.0/20
    192.168.0.0/16
  ]]

  local app_group1 = util_conf.new_app_group()
  app_group1:set_name("Base")
  app_group1:set_enabled(true)
  app_group1:set_block[[
    System
  ]]
  app_group1:set_allow[[
    C:\Programs\Skype\Phone\Skype.exe
    C:\Utils\Dev\Git\*
  ]]

  local app_group2 = util_conf.new_app_group()
  app_group2:set_name("Browser")
  app_group2:set_enabled(false)
  app_group2:set_allow[[
    C:\Utils\Firefox\Bin\firefox.exe
  ]]

  conf:add_app_group(app_group1)
  conf:add_app_group(app_group2)

  local buf = assert(mem.pointer():alloc())
  assert(conf:write(buf))

  -- Parse buffer
  local conf2 = util_conf.new_conf()
  conf2:read(buf)

  assert(conf:get_ip_include_all() == conf2:get_ip_include_all())
  assert(conf:get_ip_exclude_all() == conf2:get_ip_exclude_all())
  assert(conf:get_app_log_blocked() == conf2:get_app_log_blocked())
  assert(conf:get_app_block_all() == conf2:get_app_block_all())
  assert(conf:get_app_allow_all() == conf2:get_app_allow_all())

  assert(conf2:get_ip_include() == "")

  local ip_exclude = conf2:get_ip_exclude()
  assert(ip_exclude ~= "")

  local from_exc, to_exc = util_ip.ip4range_to_numbers(ip_exclude)
  assert(from_exc.n == 5)
  assert(from_exc[3] == sock.inet_pton("169.254.0.0", true))
  assert(to_exc[3] == sock.inet_pton("169.254.255.255", true))
  
  local conf2_app_group1 = conf2:get_app_groups()[1]
  assert(app_group1:get_name() == conf2_app_group1:get_name())
  assert(app_group1:get_enabled() == conf2_app_group1:get_enabled())
  assert(conf2_app_group1:get_block() == "System\n")
  assert(conf2_app_group1:get_allow():find("C:\\utils\\dev\\git\\*\n", 1, true))

  local conf_bin = buf:getptr()
  assert(not wipf.conf_ip_inrange(conf_bin, 0, true))
  assert(not wipf.conf_ip_inrange(conf_bin, sock.inet_pton("9.255.255.255", true)))
  assert(not wipf.conf_ip_inrange(conf_bin, sock.inet_pton("11.0.0.0", true)))
  assert(wipf.conf_ip_inrange(conf_bin, sock.inet_pton("10.0.0.0", true)))
  assert(wipf.conf_ip_inrange(conf_bin, sock.inet_pton("169.254.100.100", true)))
  assert(wipf.conf_ip_inrange(conf_bin, sock.inet_pton("192.168.255.255", true)))
  assert(not wipf.conf_ip_inrange(conf_bin, sock.inet_pton("193.0.0.0", true)))

  assert(wipf.conf_app_blocked(conf_bin, util_fs.path_to_dospath[[System]]))
  assert(not wipf.conf_app_blocked(conf_bin, util_fs.path_to_dospath[[C:\programs\skype\phone\skype.exe]]))
  assert(not wipf.conf_app_blocked(conf_bin, util_fs.path_to_dospath[[C:\utils\dev\git\bin\git.exe]]))

  print("OK")
end


print"-- I18n"
do
  local id = 'err_conf_iprange_inc'
  local text = i18n.tr_fmt(id, 3)
  assert(text ~= id)
  print("OK")
end


