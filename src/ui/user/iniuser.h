#ifndef INIUSER_H
#define INIUSER_H

#include <util/ini/mapsettings.h>

namespace HotKey {

const char *const home = "home";
const char *const programs = "programs";
const char *const options = "options";
const char *const rules = "rules";
const char *const zones = "zones";
const char *const statistics = "statistics";
const char *const graph = "graph";

const char *const filter = "filter";
const char *const blockTraffic = "blockTraffic";
const char *const blockInetTraffic = "blockInetTraffic";

const char *const filterModeAutoLearn = "filterModeAutoLearn";
const char *const filterModeAskToConnect = "filterModeAskToConnect";
const char *const filterModeBlock = "filterModeBlock";
const char *const filterModeAllow = "filterModeAllow";
const char *const filterModeIgnore = "filterModeIgnore";

const char *const appGroupModifier = "appGroupModifier";
const char *const quit = "quit";

extern const char *const list[];
extern const int listCount;

const char *const defaultValue(const char *key);

namespace Default {
const char *const filter = "Ctrl+Alt+Shift+F";
const char *const appGroupModifier = "Ctrl+Alt+Shift";
}

}

class IniUser : public MapSettings
{
public:
    explicit IniUser(Settings *settings = nullptr);

    QString defaultLanguage() const { return m_defaultLanguage; }
    void setDefaultLanguage(const QString &v) { m_defaultLanguage = v; }

    QString language() const { return valueText("base/language"); }
    void setLanguage(const QString &v) { setValue("base/language", v); }

    bool useSystemLocale() const { return valueBool("base/useSystemLocale", true); }
    void setUseSystemLocale(bool v) { setValue("base/useSystemLocale", v, true); }

    bool hotKeyEnabled() const { return valueBool("hotKey/enabled"); }
    void setHotKeyEnabled(bool v) { setValue("hotKey/enabled", v); }

    bool hotKeyGlobal() const { return valueBool("hotKey/global", true); }
    void setHotKeyGlobal(bool v) { setValue("hotKey/global", v, true); }

    QString hotKeyValue(const char *key) const;
    void setHotKeyValue(const char *key, const QString &v);

    bool splashWindowVisible() const { return valueBool("splashWindow/visible", true); }
    void setSplashWindowVisible(bool on) { setValue("splashWindow/visible", on, true); }

    bool homeAutoShowMenu() const { return valueBool("home/autoShowMenu"); }
    void setHomeAutoShowMenu(bool v) { setValue("home/autoShowMenu", v); }

    bool progNotifyMessage() const { return valueBool("prog/notifyMessage", true); }
    void setProgNotifyMessage(bool v) { setValue("prog/notifyMessage", v, true); }

    bool trayShowIcon() const { return valueBool("tray/showIcon", true); }
    void setTrayShowIcon(bool v) { setValue("tray/showIcon", v, true); }

    bool trayAnimateAlert() const { return valueBool("tray/animateAlert", true); }
    void setTrayAnimateAlert(bool v) { setValue("tray/animateAlert", v, true); }

    int trayMaxGroups(int v = 0) const { return valueInt("tray/maxGroups", v); }
    void setTrayMaxGroups(int v) { setValue("tray/maxGroups", v); }

    QString trayAction(const QString &event) const { return valueText("tray/" + event); }
    void setTrayAction(const QString &event, const QString &v) { setValue("tray/" + event, v); }

    bool confirmTrayFlags() const { return valueBool("confirm/trayFlags"); }
    void setConfirmTrayFlags(bool v) { setValue("confirm/trayFlags", v); }

    bool confirmQuit() const { return valueBool("confirm/quit", true); }
    void setConfirmQuit(bool v) { setValue("confirm/quit", v, true); }

    QRect homeWindowGeometry() const { return value("homeWindow/geometry").toRect(); }
    void setHomeWindowGeometry(const QRect &v) { setValue("homeWindow/geometry", v); }

    bool homeWindowMaximized() const { return valueBool("homeWindow/maximized"); }
    void setHomeWindowMaximized(bool on) { setValue("homeWindow/maximized", on); }

    QRect progWindowGeometry() const { return value("progWindow/geometry").toRect(); }
    void setProgWindowGeometry(const QRect &v) { setValue("progWindow/geometry", v); }

    bool progWindowMaximized() const { return valueBool("progWindow/maximized"); }
    void setProgWindowMaximized(bool on) { setValue("progWindow/maximized", on); }

    bool progAppsSortDesc() const { return valueBool("progWindow/appsSortDesc"); }
    void setProgSortDesc(bool v) { setValue("progWindow/appsSortDesc", v); }

    int progAppsSortColumn() const { return valueInt("progWindow/appsSortColumn"); }
    void setProgSortColumn(int v) { setValue("progWindow/appsSortColumn", v); }

    int progAppsHeaderVersion() const { return valueInt("progWindow/appsHeaderVersion"); }
    void setProgAppsHeaderVersion(int v) { setValue("progWindow/appsHeaderVersion", v); }

    QByteArray progAppsHeader() const { return valueByteArray("progWindow/appsHeader"); }
    void setProgAppsHeader(const QByteArray &v) { setValue("progWindow/appsHeader", v); }

    QRect progAlertWindowGeometry() const { return value("progAlertWindow/geometry").toRect(); }
    void setProgAlertWindowGeometry(const QRect &v) { setValue("progAlertWindow/geometry", v); }

    bool progAlertWindowMaximized() const { return valueBool("progAlertWindow/maximized"); }
    void setProgAlertWindowMaximized(bool on) { setValue("progAlertWindow/maximized", on); }

    bool progAlertWindowAutoShow() const { return valueBool("progAlertWindow/autoShow", true); }
    void setProgAlertWindowAutoShow(bool on) { setValue("progAlertWindow/autoShow", on); }

    bool progAlertWindowAlwaysOnTop() const { return valueBool("progAlertWindow/alwaysOnTop"); }
    void setProgAlertWindowAlwaysOnTop(bool on) { setValue("progAlertWindow/alwaysOnTop", on); }

    QRect optWindowGeometry() const { return value("optWindow/geometry").toRect(); }
    void setOptWindowGeometry(const QRect &v) { setValue("optWindow/geometry", v); }

    bool optWindowMaximized() const { return valueBool("optWindow/maximized"); }
    void setOptWindowMaximized(bool on) { setValue("optWindow/maximized", on); }

    QByteArray optWindowAddrSplit() const { return valueByteArray("optWindow/addrSplit"); }
    void setOptWindowAddrSplit(const QByteArray &v) { setValue("optWindow/addrSplit", v); }

    QByteArray optWindowAppsSplit() const { return valueByteArray("optWindow/appsSplit"); }
    void setOptWindowAppsSplit(const QByteArray &v) { setValue("optWindow/appsSplit", v); }

    QRect ruleWindowGeometry() const { return value("ruleWindow/geometry").toRect(); }
    void setRuleWindowGeometry(const QRect &v) { setValue("ruleWindow/geometry", v); }

    bool ruleWindowMaximized() const { return valueBool("ruleWindow/maximized"); }
    void setRuleWindowMaximized(bool on) { setValue("ruleWindow/maximized", on); }

    int rulesHeaderVersion() const { return valueInt("ruleWindow/rulesHeaderVersion"); }
    void setRulesHeaderVersion(int v) { setValue("ruleWindow/rulesHeaderVersion", v); }

    QByteArray rulesHeader() const { return valueByteArray("ruleWindow/rulesHeader"); }
    void setRulesHeader(const QByteArray &v) { setValue("ruleWindow/rulesHeader", v); }

    int rulesExpanded() const { return valueInt("ruleWindow/rulesExpanded", 0x01); }
    void setRulesExpanded(int v) { setValue("ruleWindow/rulesExpanded", v); }

    QRect serviceWindowGeometry() const { return value("serviceWindow/geometry").toRect(); }
    void setServiceWindowGeometry(const QRect &v) { setValue("serviceWindow/geometry", v); }

    bool serviceWindowMaximized() const { return valueBool("serviceWindow/maximized"); }
    void setServiceWindowMaximized(bool on) { setValue("serviceWindow/maximized", on); }

    int servicesHeaderVersion() const { return valueInt("serviceWindow/servicesHeaderVersion"); }
    void setServicesHeaderVersion(int v) { setValue("serviceWindow/servicesHeaderVersion", v); }

    QByteArray servicesHeader() const { return valueByteArray("serviceWindow/servicesHeader"); }
    void setServicesHeader(const QByteArray &v) { setValue("serviceWindow/servicesHeader", v); }

    QRect zoneWindowGeometry() const { return value("zoneWindow/geometry").toRect(); }
    void setZoneWindowGeometry(const QRect &v) { setValue("zoneWindow/geometry", v); }

    bool zoneWindowMaximized() const { return valueBool("zoneWindow/maximized"); }
    void setZoneWindowMaximized(bool on) { setValue("zoneWindow/maximized", on); }

    int zonesHeaderVersion() const { return valueInt("zoneWindow/zonesHeaderVersion"); }
    void setZonesHeaderVersion(int v) { setValue("zoneWindow/zonesHeaderVersion", v); }

    QByteArray zonesHeader() const { return valueByteArray("zoneWindow/zonesHeader"); }
    void setZonesHeader(const QByteArray &v) { setValue("zoneWindow/zonesHeader", v); }

    constexpr bool graphWindowHideOnCloseDefault() const { return false; }
    bool graphWindowHideOnClose() const { return valueBool("graphWindow/hideOnClose"); }
    void setGraphWindowHideOnClose(bool on) { setValue("graphWindow/hideOnClose", on); }

    bool graphWindowVisible() const { return valueBool("graphWindow/visible"); }
    void setGraphWindowVisible(bool on) { setValue("graphWindow/visible", on); }

    QRect graphWindowGeometry() const { return value("graphWindow/geometry").toRect(); }
    void setGraphWindowGeometry(const QRect &v) { setValue("graphWindow/geometry", v); }

    bool graphWindowMaximized() const { return valueBool("graphWindow/maximized"); }
    void setGraphWindowMaximized(bool on) { setValue("graphWindow/maximized", on); }

    QRect statWindowGeometry() const { return value("statWindow/geometry").toRect(); }
    void setStatWindowGeometry(const QRect &v) { setValue("statWindow/geometry", v); }

    bool statWindowMaximized() const { return valueBool("statWindow/maximized"); }
    void setStatWindowMaximized(bool on) { setValue("statWindow/maximized", on); }

    int statTrafUnit() const { return valueInt("statWindow/trafUnit"); }
    void setStatTrafUnit(int v) { setValue("statWindow/trafUnit", v); }

    QByteArray statWindowTrafSplit() const { return valueByteArray("statWindow/trafSplit"); }
    void setStatWindowTrafSplit(const QByteArray &v) { setValue("statWindow/trafSplit", v); }

    int connListHeaderVersion() const { return valueInt("statWindow/connListHeaderVersion"); }
    void setConnListHeaderVersion(int v) { setValue("statWindow/connListHeaderVersion", v); }

    QByteArray connListHeader() const { return valueByteArray("statWindow/connListHeader"); }
    void setConnListHeader(const QByteArray &v) { setValue("statWindow/connListHeader", v); }

    bool statAutoScroll() const { return valueBool("statWindow/autoScroll"); }
    void setStatAutoScroll(bool on) { setValue("statWindow/autoScroll", on); }

    bool statShowHostNames() const { return valueBool("statWindow/showHostNames"); }
    void setStatShowHostNames(bool on) { setValue("statWindow/showHostNames", on); }

public:
    void saveDefaultIni();

private:
    QString m_defaultLanguage;
};

#endif // INIUSER_H
