#ifndef USERSETTINGS_H
#define USERSETTINGS_H

#include <util/ini/settings.h>
#include <util/ioc/iocservice.h>

#include "iniuser.h"

class UserSettings : public Settings, public IocService
{
    Q_OBJECT

public:
    explicit UserSettings(QObject *parent = nullptr);

    IniUser &iniUser() { return m_iniUser; }
    const IniUser &iniUser() const { return m_iniUser; }

    void setUp() override;

protected:
    void migrateQtVerOnLoad() override;
    void migrateQtVerOnWrite() override;

    void migrateIniOnLoad() override;
    void migrateIniOnWrite() override;

private:
    IniUser m_iniUser;
};

#endif // USERSETTINGS_H
