#include "iniuser.h"

IniUser::IniUser(Settings *settings) : MapSettings(settings) { }

void IniUser::saveDefaultIni()
{
    setLanguage(defaultLanguage());

    save();
}
