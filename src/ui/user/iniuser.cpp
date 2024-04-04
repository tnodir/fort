#include "iniuser.h"

namespace HotKey {

const char *const filterModes[] = {
    filterModeAutoLearn,
    filterModeAskToConnect,
    filterModeBlock,
    filterModeAllow,
    filterModeIgnore,
};

}

IniUser::IniUser(Settings *settings) : MapSettings(settings) { }

void IniUser::saveDefaultIni()
{
    setLanguage(defaultLanguage());

    saveAndClear();
}
