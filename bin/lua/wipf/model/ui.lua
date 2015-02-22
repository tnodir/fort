-- WIPF UI Model

local observer = require"wipf.util.observer"
local ui_window = require"wipf.ui.window"


local model_ui = {}


function model_ui:handle_apply_conf(window, conf)
end


-- Create connections
observer.connect(ui_window.apply_conf, model_ui, model_ui.handle_apply_conf)


return model_ui
