-- WIPF Log Tests

local sys = require"sys"
local wipf = require"wipflua"


print"-- Log Write"
do
  local path = "/test"
  local outlen = 12 + #path
  local bufsize = outlen + 4
  local buf = assert(sys.mem.pointer(bufsize))

  buf:memset(0, bufsize)
  buf[outlen] = 127
  assert(wipf.log_write(buf:getptr(), 1, 2, path) == outlen)
  assert(buf:tostring(#path, 12) == path)
  assert(buf[outlen] == 127)

  local inlen, ip, pid, inpath = wipf.log_read(buf:getptr())
  assert(inlen == outlen and ip == 1 and pid == 2 and inpath == path)

  print("OK")
end


