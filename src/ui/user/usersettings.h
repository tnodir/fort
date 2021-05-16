#ifndef USERSETTINGS_H
#define USERSETTINGS_H

#include "../util/ini/settings.h"
#include "iniuser.h"

class FortSettings;

class UserSettings : public Settings
{
    Q_OBJECT

public:
    explicit UserSettings(QObject *parent = nullptr);

    IniUser &ini() { return m_ini; }
    const IniUser &ini() const { return m_ini; }

    void initialize(FortSettings *settings);

private:
    IniUser m_ini;
};

#endif // USERSETTINGS_H
