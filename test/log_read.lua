-- WIPF Log Reader

local sys = require"sys"
local wipf = require"wipflua"


local device = assert(sys.handle():open(wipf.device_name(), "rw"))

local BUFSIZ = wipf.buffer_size_max()
local buf = assert(sys.mem.pointer(BUFSIZ))

assert(buf:write("test config"))

assert(device:ioctl(wipf.ioctl_setconf(), buf))

print("buffer size:", buf:length())

while true do
  assert(device:ioctl(wipf.ioctl_getlog(), nil, buf))
  print("log size:", buf:seek())
  buf:seek(0)
end
