#include "iniuser.h"

IniUser::IniUser(Settings *settings) : MapSettings(settings) { }

void IniUser::setDefaultLanguage(const QString &v)
{
    m_defaultLanguage = v;

    if (language().isEmpty()) {
        setLanguage(defaultLanguage());
        save();
    }
}
