-- WIPF Internationalization Utilities

local util_fs = require"wipf.util.fs"


local i18n = {}

local current_lang, lang_strings


function i18n.set_current_lang(lang)
  if current_lang == lang then return end

  current_lang = lang
  lang_strings = assert(util_fs.sandbox("lua/wipf/lang/" .. lang .. ".lua"))
end

function i18n.get_current_lang()
  return current_lang
end

-- Get translation text for l10n identifier
function i18n.tr(id)
  return lang_strings[id] or id
end

-- Get formatted translation text for l10n identifier
function i18n.tr_fmt(id, ...)
  local s = lang_strings[id]
  if not s then
    return id
  end

  return string.format(s, ...)
end


-- Set default language English
i18n.set_current_lang("en")


return i18n
