-- WIPF File-System Utilities

local sys = require"sys"

local win32 = sys.win32


-- Get Process's Native path by identifier
local function pid_dospath(id)
  local pid = sys.pid(id, true)
  local path = (pid and pid:path()) or ""
  pid:close()
  return path
end

-- Convert DOS device name to drive letter (A: .. Z:)
local dosname_to_drive

-- Convert drive letter (A: .. Z:) to DOS device name
local drive_to_dosname

do
  local drives

  local function fill_drives()
    drives = {}
    for drive in sys.dir("/") do
      local dos_name = win32.drive_dosname(drive):lower()
      drives[dos_name] = drive
      drives[drive] = dos_name
    end
  end

  local function get_value(key)
    if not drives or not drives[key] then
      fill_drives()
    end

    return drives[key] or ""
  end

  dosname_to_drive = function (dos_name)
    return get_value(dos_name:lower())
  end

  drive_to_dosname = function (drive)
    return get_value(drive:upper())
  end
end

-- Convert Native path to Win32 path
local function dospath_to_path(dos_path)
  local dos_name, sub_path = string.match(dos_path, [[(\[^\]+\[^\]+)(\.+)]])
  if not dos_name then
    return dos_path
  end
  return dosname_to_drive(dos_name) .. sub_path
end

-- Convert Win32 path to Native path
local function path_to_dospath(path)
  local drive, sub_path = string.match(path, [[(%a:)(\.+)]])
  if not drive then
    return path
  end
  return drive_to_dosname(drive) .. sub_path
end

-- Load file, run it in sandbox and return it's globals in a table
function sandbox(path)
  local chunk, err_msg = loadfile(path)
  if not chunk then
    return nil, err_msg
  end
  local env = setmetatable({}, nil)
  setfenv(chunk, env)
  chunk()
  return env
end


return {
  pid_dospath		= pid_dospath,
  dospath_to_path	= dospath_to_path,
  path_to_dospath	= path_to_dospath,
  sandbox		= sandbox,
}
