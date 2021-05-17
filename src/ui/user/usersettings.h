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

    IniUser &iniUser() { return m_iniUser; }
    const IniUser &iniUser() const { return m_iniUser; }

    void initialize(FortSettings *settings);

protected:
    void migrateIniOnStartup() override;
    void migrateIniOnWrite() override;

private:
    IniUser m_iniUser;
};

#endif // USERSETTINGS_H
