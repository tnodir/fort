-- WIPF GUI: Main window

local iup = require"iuplua"

local i18n = require"wipf.util.i18n"


local ui_window = {}

local window, menu


local function window_show()
  window:show()
end

local function window_quit()
  window:destroy()
  return iup.CLOSE
end

local function menu_show()
  menu:popup(iup.MOUSEPOS, iup.MOUSEPOS)
end

local function on_trayclick(self, b, press)
  if press == 0 then
    if b == 1 then
      window_show()
    else
      menu_show()
    end
  end
  return iup.DEFAULT
end

local function on_windowclose()
  window:hide()
  return iup.IGNORE
end


-- Menu
local function menu_init()

  if menu then menu:destroy() end

  menu = iup.menu{
    iup.item{
      title = i18n.tr("ui_tray_show"),
      action = window_show
    },
    iup.separator{},
    iup.item{
      title = i18n.tr("ui_tray_quit"),
      action = window_quit
    }
  }
end


function ui_window.init(window_title)

  -- Initialize Window
  window = iup.dialog{
    iup.label{title = "Test Main Window"};
    title = window_title, tray = "YES", hidetaskbar = "YES",
    icon = "images/shield.ico", trayimage = "images/shield.ico",
    traytip = window_title,
    trayclick_cb = on_trayclick, close_cb = on_windowclose,
    copydata_cb = window_show
  }

  -- Initialize Menu
  menu_init()

  window:map()
end


return ui_window
