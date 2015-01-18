-- WIPF Utilities

local sys = require"sys"

local win32 = sys.win32


-- Get Process's Native path by identifier
function pid_dospath(id)
  local pid = sys.pid(id, true)
  local path = (pid and pid:path()) or ""
  pid:close()
  return path
end

-- Convert DOS device name to drive letter (A: .. Z:)
local dosname_to_drive
do
  local drives

  local function fill_drives()
    drives = {}
    for drive in sys.dir("/") do
      local dos_name = win32.drive_dosname(drive)
      drives[dos_name:lower()] = drive
    end
  end

  dosname_to_drive = function (dos_name)
    if not drives then
      fill_drives()
    end

    return drives[dos_name:lower()] or "?:"
  end
end

-- Convert Native path to Win32 path
local function dospath_to_path(dos_path)
  local dos_name, path = string.match(dos_path, [[(\[^\]+\[^\]+)(\.+)]])
  if not dos_name then
    return dos_path
  end
  return dosname_to_drive(dos_name) .. path
end

-- Convert IPv4 ranges in text to from & to arrays with numbers
local function ip4range_to_numbers(text)
  local from, to = {}, {}

  return from, to
end


return {
  dosname_to_drive	= dosname_to_drive,
  dospath_to_path	= dospath_to_path,
  ip4range_to_numbers	= ip4range_to_numbers,
}
