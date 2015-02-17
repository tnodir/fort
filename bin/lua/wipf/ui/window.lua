-- WIPF GUI: Main window

local iup = require"iuplua"

local i18n = require"wipf.util.i18n"


local ui_window = {}

local window, menu


local function window_show()
  window:show()
end

local function window_hide()
  window:hide()
end

local function window_quit()
  window:destroy()
  return iup.CLOSE
end

local function menu_show()
  menu:popup(iup.MOUSEPOS, iup.MOUSEPOS)
end

local function on_tray_click(self, b, press)
  if press == 1 then
    if b == 1 then
      window_show()
    else
      menu_show()
    end
  end
  return iup.DEFAULT
end

local function on_window_close()
  window_hide()
  return iup.IGNORE
end

local function on_window_apply()
  window_hide()
end

local function on_window_cancel()
  window_hide()
end


local function create_toggle(title, action_cb)
  return iup.toggle{
    title = title, padding = 8,
    action = action_cb
  }
end

local function create_button(title, action_cb)
  return iup.button{
    title = title, padding = 8,
    action = action_cb
  }
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
  local btn_apply = create_button(i18n.tr("ui_win_ok"), on_window_apply)
  local btn_cancel = create_button(i18n.tr("ui_win_cancel"), on_window_cancel)

  window = iup.dialog{
    iup.vbox{
      iup.tabs{
        -- Options
        iup.vbox{
          create_toggle(i18n.tr("ui_opt_autostart"), on_opt_autostart),
          create_toggle(i18n.tr("ui_opt_filter_disabled"), on_opt_filter_disabled);
          tabtitle = i18n.tr("ui_tab_options")
        },
        -- IPv4 Addresses
        iup.hbox{
          iup.multiline{expand = "YES"},
          iup.multiline{expand = "YES"};
          tabtitle = i18n.tr("ui_tab_addresses")
        },
        -- Applications
        iup.vbox{
          tabtitle = i18n.tr("ui_tab_apps")
        }
      },
      iup.hbox{
        iup.fill{}, btn_apply, btn_cancel;
        gap = 10
      };
      gap = 2, margin = "2x2"
    };
    title = window_title, traytip = window_title,
    shrink = "YES", hidetaskbar = "YES", tray = "YES",
    icon = "images/shield.ico", trayimage = "images/shield.ico",
    size = "HALFxHALF", minsize = "750x750", saveunder = "NO",
    defaultenter = btn_apply, defaultesc = btn_cancel,
    trayclick_cb = on_tray_click, close_cb = on_window_close,
    copydata_cb = window_show
  }

  -- Initialize Menu
  menu_init()

  window:map()
end


return ui_window
