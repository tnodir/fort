-- WIPF Log Reader

local sys = require"sys"
local wipf = require"wipflua"


function print_logs(buf)
  local size = buf:seek()
  local ptr = buf:getptr()
  local off = 0

  while off < size do
    local len, ip, pid, path = wipf.log_read(ptr, off)
    off = off + len
    print(ip, pid, path)
  end

  buf:seek(0)
end


local device = assert(sys.handle():open(wipf.device_name(), "rw"))

local BUFSIZ = wipf.buffer_size()
local buf = assert(sys.mem.pointer(BUFSIZ))

assert(buf:write("test config"))

assert(device:ioctl(wipf.ioctl_setconf(), buf))

print("buffer size:", buf:length())

while true do
  assert(device:ioctl(wipf.ioctl_getlog(), nil, buf))
  print_logs(buf)
end
