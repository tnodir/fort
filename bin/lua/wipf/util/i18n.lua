-- WIPF Internationalization Utilities

local util_fs = require"wipf/util/fs"


local current_lang, lang_strings


local function set_current_lang(lang)
  current_lang = lang
  lang_strings = assert(util_fs.sandbox("lua/wipf/lang/" .. lang .. ".lua"))
end

local function get_current_lang()
  return current_lang
end

-- Get translation text for l10n identifier
local function tr(id)
  return lang_strings[id] or id
end

-- Get formatted translation text for l10n identifier
local function tr_fmt(id, ...)
  local s = lang_strings[id]
  if not s then
    return id
  end

  return string.format(s, ...)
end


-- Set default language English
set_current_lang("en")


return {
  set_current_lang	= set_current_lang,
  get_current_lang	= get_current_lang,
  tr			= tr,
  tr_fmt		= tr_fmt,
}
