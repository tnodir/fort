-- WIPF Log Reader

local sys = require"sys"
local sock = require"sys.sock"

local wipf = require"wipflua"
local util_conf = require"wipf.util.conf"
local util_fs = require"wipf.util.fs"

local mem, win32 = sys.mem, sys.win32


function set_conf(device)
  local conf = util_conf.new_conf()

  conf:set_ip_include_all(true)
  conf:set_ip_exclude_all(false)

  conf:set_app_log_blocked(true)
  conf:set_app_block_all(true)
  conf:set_app_allow_all(false)

  conf:set_ip_exclude[[
    10.0.0.0/24
    127.0.0.0/24
    169.254.0.0/16
    172.16.0.0/20
    192.168.0.0/16
  ]]

  local buf = assert(mem.pointer():alloc())
  assert(conf:write(buf))

  return device:ioctl(wipf.ioctl_setconf(), buf)
end

function print_logs(buf)
  local size = buf:seek()
  local ptr = buf:getptr()
  local off = 0

  while off < size do
    local len, ip, pid, dos_path = wipf.log_read(ptr, off)
    local ip_str = sock.inet_ntop(ip)

    if not dos_path then
      dos_path = util_fs.pid_dospath(pid)
    end

    local path = util_fs.dospath_to_path(dos_path)

    print(ip_str, pid, path)

    off = off + len
  end

  buf:seek(0)
end


local device = assert(sys.handle():open(wipf.device_name(), "rw"))

assert(set_conf(device))

local BUFSIZ = wipf.buffer_size()
local buf = assert(sys.mem.pointer(BUFSIZ))

while true do
  assert(device:ioctl(wipf.ioctl_getlog(), nil, buf))
  print_logs(buf)
end
