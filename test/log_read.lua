-- WIPF Log Reader

local sys = require"sys"
local wipf = require"wipflua"


local device = assert(sys.handle():open(wipf.device_name()))

local BUFSIZ = wipf.buffer_size_max()
local buf = assert(sys.mem.pointer(BUFSIZ))

while true do
  assert(device:ioctl(wipf.ioctl_getlog(), nil, buf))
  print("size:", buf:seek())
  buf:seek(0)
end
