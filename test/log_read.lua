-- WIPF Log Reader

local sys = require"sys"
local sock = require"sys.sock"

local wipf = require"wipflua"
local wipf_util = require"wipf/util"


function print_logs(buf)
  local size = buf:seek()
  local ptr = buf:getptr()
  local off = 0

  while off < size do
    local len, ip, pid, dos_path = wipf.log_read(ptr, off)
    local ip_str = sock.inet_ntop(ip)

    if not dos_path then
      dos_path = wipf_util.pid_dospath(pid)
    end

    local path = wipf_util.dospath_to_path(dos_path)

    print(ip_str, pid, path)

    off = off + len
  end

  buf:seek(0)
end


local device = assert(sys.handle():open(wipf.device_name(), "rw"))

local BUFSIZ = wipf.buffer_size()
local buf = assert(sys.mem.pointer(BUFSIZ))

assert(buf:write("test config"))
assert(device:ioctl(wipf.ioctl_setconf(), buf))

while true do
  assert(device:ioctl(wipf.ioctl_getlog(), nil, buf))
  print_logs(buf)
end
