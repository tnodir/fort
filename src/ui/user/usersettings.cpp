#include "usersettings.h"

#include <fort_version.h>

#include "../fortsettings.h"

UserSettings::UserSettings(QObject *parent) : Settings(parent), m_ini(this) { }

void UserSettings::initialize(FortSettings *settings)
{
    setupIni(settings->userPath() + APP_BASE + ".user.ini");

    ini().setDefaultLanguage(settings->defaultLanguage());
}
