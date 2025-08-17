#include "usersettings.h"

#include <fort_version.h>

#include <fortsettings.h>
#include <util/ioc/ioccontainer.h>

UserSettings::UserSettings(QObject *parent) : Settings(parent), m_iniUser(this) { }

void UserSettings::setUp()
{
    auto settings = IoC<FortSettings>();

    iniUser().setDefaultLanguage(settings->defaultLanguage());

    setupIni(settings->userPath() + APP_BASE + ".user.ini");
}

void UserSettings::migrateQtVerOnLoad()
{
    if (checkQtVersionIsValid())
        return;

    const QVariant emptyArray = QByteArray();

    setCacheValue("progWindow/appsHeader", emptyArray);
    setCacheValue("ruleWindow/rulesHeader", emptyArray);
    setCacheValue("serviceWindow/servicesHeader", emptyArray);
    setCacheValue("zoneWindow/zonesHeader", emptyArray);
    setCacheValue("statWindow/connListHeader", emptyArray);
}

void UserSettings::migrateQtVerOnWrite()
{
    if (checkQtVersionIsEqual())
        return;

    Settings::migrateQtVerOnWrite();
}

void UserSettings::migrateIniOnLoad()
{
    if (!iniExists()) {
        iniUser().saveDefaultIni();
        return;
    }

    int version;
    if (checkIniVersion(version))
        return;

    Settings::migrateIniOnLoad();

    // COMPAT: v3.4.0: .ini ~> .user.ini
    if (version < 0x030400) {
        setCacheValue("statWindow/trafUnit", ini()->value("stat/trafUnit"));
        setCacheValue("statWindow/trafSplit", ini()->value("optWindow/statSplit"));
        setCacheValue("statWindow/geometry", ini()->value("connWindow/geometry"));
        setCacheValue("statWindow/connListHeader", ini()->value("connWindow/connListHeader"));
        setCacheValue("statWindow/connListHeaderVersion",
                ini()->value("connWindow/connListHeaderVersion"));
        setCacheValue("statWindow/autoScroll", ini()->value("connWindow/autoScroll"));
        setCacheValue("statWindow/showHostNames", ini()->value("connWindow/showHostNames"));
        setCacheValue("statWindow/maximized", ini()->value("connWindow/maximized"));
    }

    // COMPAT: v3.9.11
    if (version < 0x030911) {
        setCacheValue("prog/notifyMessage", ini()->value("tray/alertMessage"));
    }

    // COMPAT: v3.13.6
    if (version < 0x031306) {
        setCacheValue("homeWindow/autoShowMenu", ini()->value("home/autoShowMenu"));
    }
}

void UserSettings::migrateIniOnWrite()
{
    int version;
    if (checkIniVersionOnWrite(version))
        return;

    Settings::migrateIniOnWrite();

    // COMPAT: v3.4.0: .ini ~> .user.ini
    if (version < 0x030400) {
        removeIniKey("base/debug");
        removeIniKey("base/console");
        removeIniKey("base/passwordHash");
        removeIniKey("confFlags");
        removeIniKey("stat");
        removeIniKey("quota");
        removeIniKey("optWindow/statSplit");
        removeIniKey("connWindow");
        ini()->setValue("statWindow/geometry", cacheValue("statWindow/geometry"));
        ini()->setValue("statWindow/trafUnit", cacheValue("statWindow/trafUnit"));
        ini()->setValue("statWindow/trafSplit", cacheValue("statWindow/trafSplit"));
        ini()->setValue("statWindow/connListHeader", cacheValue("statWindow/connListHeader"));
        ini()->setValue(
                "statWindow/connListHeaderVersion", cacheValue("statWindow/connListHeaderVersion"));
        ini()->setValue("statWindow/autoScroll", cacheValue("statWindow/autoScroll"));
        ini()->setValue("statWindow/showHostNames", cacheValue("statWindow/showHostNames"));
        ini()->setValue("statWindow/maximized", cacheValue("statWindow/maximized"));
    }

    // COMPAT: v3.6.2: Remove "Dark Mode"
    if (version < 0x030602) {
        removeIniKey("base/isDarkMode");
    }

    // COMPAT: v3.6.8: Rules tab ~> Policies window
    if (version < 0x030608) {
        removeIniKey("optWindow/rulesPresetSplit");
        removeIniKey("optWindow/rulesGlobalSplit");
        removeIniKey("optWindow/rulesSplit");
        removeIniKey("optWindow/rulesSplitVersion");
    }

    // COMPAT: v3.9.11
    if (version < 0x030911) {
        removeIniKey("tray/alertMessage");
    }

    // COMPAT: v3.13.6
    if (version < 0x031306) {
        removeIniKey("home/autoShowMenu");
        ini()->setValue("homeWindow/autoShowMenu", cacheValue("homeWindow/autoShowMenu"));
    }
}
