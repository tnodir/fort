#ifndef INIUSER_H
#define INIUSER_H

#include <util/ini/mapsettings.h>

namespace HotKey {

const char *const home = "home";
const char *const programs = "programs";
const char *const options = "options";
const char *const rules = "rules";
const char *const zones = "zones";
const char *const groups = "groups";
const char *const services = "services";
const char *const statistics = "statistics";
const char *const graph = "graph";

const char *const filter = "filter";
const char *const snoozeAlerts = "snoozeAlerts";

const char *const blockTrafficOff = "blockTrafficOff";
const char *const blockTraffic = "blockTraffic";
const char *const blockInetLanTraffic = "blockInetLanTraffic";
const char *const blockLanTraffic = "blockLanTraffic";
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

    bool excludeFromCapture() const { return valueBool("base/excludeFromCapture"); }
    void setExcludeFromCapture(bool v) { setValue("base/excludeFromCapture", v); }

    QString theme() const { return valueText("base/theme"); }
    void setTheme(const QString &v) { setValue("base/theme", v); }

    static QString styleDefault() { return "Fusion"; }
    QString style() const { return valueText("base/style", styleDefault()); }
    void setStyle(const QString &v) { setValue("base/style", v, styleDefault()); }

    bool hotKeyEnabled() const { return valueBool("hotKey/enabled"); }
    void setHotKeyEnabled(bool v) { setValue("hotKey/enabled", v); }

    bool hotKeyGlobal() const { return valueBool("hotKey/global", true); }
    void setHotKeyGlobal(bool v) { setValue("hotKey/global", v, true); }

    QString hotKeyValue(const char *key) const;
    void setHotKeyValue(const char *key, const QString &v);

    bool splashWindowVisible() const { return valueBool("splashWindow/visible", true); }
    void setSplashWindowVisible(bool on) { setValue("splashWindow/visible", on, true); }

    bool updateWindowIcons() const { return valueBool("splashWindow/updateWindowIcons"); }
    void setUpdateWindowIcons(bool on) { setValue("splashWindow/updateWindowIcons", on); }

    bool progNotifyMessage() const { return valueBool("prog/notifyMessage", true); }
    void setProgNotifyMessage(bool v) { setValue("prog/notifyMessage", v, true); }

    bool progAlertSound() const { return valueBool("prog/soundAlert", true); }
    void setProgAlertSound(bool v) { setValue("prog/soundAlert", v, true); }

    bool progSnoozeAlerts() const { return valueBool("prog/snoozeAlerts"); }
    void setProgSnoozeAlerts(bool v) { setValue("prog/snoozeAlerts", v); }

    bool trayShowIcon() const { return valueBool("tray/showIcon", true); }
    void setTrayShowIcon(bool v) { setValue("tray/showIcon", v, true); }

    bool trayShowAlert() const { return valueBool("tray/showAlert", true); }
    void setTrayShowAlert(bool v) { setValue("tray/showAlert", v, true); }

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

    static QString homeWindowGroup() { return "homeWindow"; }

    QRect homeWindowGeometry() const { return value("homeWindow/geometry").toRect(); }
    void setHomeWindowGeometry(const QRect &v) { setValue("homeWindow/geometry", v); }

    bool homeWindowMaximized() const { return valueBool("homeWindow/maximized"); }
    void setHomeWindowMaximized(bool on) { setValue("homeWindow/maximized", on); }

    bool homeWindowAutoShowWindow() const { return valueBool("homeWindow/autoShowWindow", true); }
    void setHomeWindowAutoShowWindow(bool v) { setValue("homeWindow/autoShowWindow", v, true); }

    bool homeWindowAutoShowMenu() const { return valueBool("homeWindow/autoShowMenu"); }
    void setHomeWindowAutoShowMenu(bool v) { setValue("homeWindow/autoShowMenu", v); }

    static QString progWindowGroup() { return "progWindow"; }

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

    bool progWindowAutoClearAlerts() const { return valueBool("progWindow/autoClearAlerts"); }
    void setProgWindowAutoClearAlerts(bool on) { setValue("progWindow/autoClearAlerts", on); }

    int progWindowSortState() const { return valueInt("progWindow/sortState"); }
    void setProgWindowSortState(int v) { setValue("progWindow/sortState", v); }

    static QString progAlertWindowGroup() { return "progAlertWindow"; }

    QRect progAlertWindowGeometry() const { return value("progAlertWindow/geometry").toRect(); }
    void setProgAlertWindowGeometry(const QRect &v) { setValue("progAlertWindow/geometry", v); }

    bool progAlertWindowMaximized() const { return valueBool("progAlertWindow/maximized"); }
    void setProgAlertWindowMaximized(bool on) { setValue("progAlertWindow/maximized", on); }

    bool progAlertWindowAutoShow() const { return valueBool("progAlertWindow/autoShow", true); }
    void setProgAlertWindowAutoShow(bool on) { setValue("progAlertWindow/autoShow", on); }

    bool progAlertWindowAutoLearn() const { return valueBool("progAlertWindow/modeAutoLearn"); }
    void setProgAlertWindowAutoLearn(bool on) { setValue("progAlertWindow/modeAutoLearn", on); }

    bool progAlertWindowBlockAll() const { return valueBool("progAlertWindow/modeBlockAll", true); }
    void setProgAlertWindowBlockAll(bool on) { setValue("progAlertWindow/modeBlockAll", on, true); }

    bool progAlertWindowAllowAll() const { return valueBool("progAlertWindow/modeAllowAll", true); }
    void setProgAlertWindowAllowAll(bool on) { setValue("progAlertWindow/modeAllowAll", on, true); }

    bool progAlertWindowAlwaysOnTop() const
    {
        return valueBool("progAlertWindow/alwaysOnTop", true);
    }
    void setProgAlertWindowAlwaysOnTop(bool on) { setValue("progAlertWindow/alwaysOnTop", on); }

    bool progAlertWindowAutoActive() const { return valueBool("progAlertWindow/autoActive"); }
    void setProgAlertWindowAutoActive(bool on) { setValue("progAlertWindow/autoActive", on); }

    static QString progAlertWindowTimedActionMinutesKey()
    {
        return "progAlertWindow/timedActionMinutes";
    }

    static QString progAlertWindowTimedRemoveMinutesKey()
    {
        return "progAlertWindow/timedRemoveMinutes";
    }

    static QString optWindowGroup() { return "optWindow"; }

    QRect optWindowGeometry() const { return value("optWindow/geometry").toRect(); }
    void setOptWindowGeometry(const QRect &v) { setValue("optWindow/geometry", v); }

    bool optWindowMaximized() const { return valueBool("optWindow/maximized"); }
    void setOptWindowMaximized(bool on) { setValue("optWindow/maximized", on); }

    QByteArray optWindowAddrSplit() const { return valueByteArray("optWindow/addrSplit"); }
    void setOptWindowAddrSplit(const QByteArray &v) { setValue("optWindow/addrSplit", v); }

    QByteArray optWindowAppsSplit() const { return valueByteArray("optWindow/appsSplit"); }
    void setOptWindowAppsSplit(const QByteArray &v) { setValue("optWindow/appsSplit", v); }

    static QString ruleWindowGroup() { return "ruleWindow"; }

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

    static QString serviceWindowGroup() { return "serviceWindow"; }

    QRect serviceWindowGeometry() const { return value("serviceWindow/geometry").toRect(); }
    void setServiceWindowGeometry(const QRect &v) { setValue("serviceWindow/geometry", v); }

    bool serviceWindowMaximized() const { return valueBool("serviceWindow/maximized"); }
    void setServiceWindowMaximized(bool on) { setValue("serviceWindow/maximized", on); }

    int servicesHeaderVersion() const { return valueInt("serviceWindow/servicesHeaderVersion"); }
    void setServicesHeaderVersion(int v) { setValue("serviceWindow/servicesHeaderVersion", v); }

    QByteArray servicesHeader() const { return valueByteArray("serviceWindow/servicesHeader"); }
    void setServicesHeader(const QByteArray &v) { setValue("serviceWindow/servicesHeader", v); }

    static QString zoneWindowGroup() { return "zoneWindow"; }

    QRect zoneWindowGeometry() const { return value("zoneWindow/geometry").toRect(); }
    void setZoneWindowGeometry(const QRect &v) { setValue("zoneWindow/geometry", v); }

    bool zoneWindowMaximized() const { return valueBool("zoneWindow/maximized"); }
    void setZoneWindowMaximized(bool on) { setValue("zoneWindow/maximized", on); }

    int zonesHeaderVersion() const { return valueInt("zoneWindow/zonesHeaderVersion"); }
    void setZonesHeaderVersion(int v) { setValue("zoneWindow/zonesHeaderVersion", v); }

    QByteArray zonesHeader() const { return valueByteArray("zoneWindow/zonesHeader"); }
    void setZonesHeader(const QByteArray &v) { setValue("zoneWindow/zonesHeader", v); }

    static QString graphWindowGroup() { return "graphWindow"; }

    constexpr bool graphWindowHideOnCloseDefault() const { return false; }
    bool graphWindowHideOnClose() const { return valueBool("graphWindow/hideOnClose"); }
    void setGraphWindowHideOnClose(bool on) { setValue("graphWindow/hideOnClose", on); }

    bool graphWindowVisible() const { return valueBool("graphWindow/visible"); }
    void setGraphWindowVisible(bool on) { setValue("graphWindow/visible", on); }

    QRect graphWindowGeometry() const { return value("graphWindow/geometry").toRect(); }
    void setGraphWindowGeometry(const QRect &v) { setValue("graphWindow/geometry", v); }

    bool graphWindowMaximized() const { return valueBool("graphWindow/maximized"); }
    void setGraphWindowMaximized(bool on) { setValue("graphWindow/maximized", on); }

    constexpr bool graphWindowAlwaysOnTopDefault() const { return true; }
    bool graphWindowAlwaysOnTop() const { return valueBool("graphWindow/alwaysOnTop", true); }
    void setGraphWindowAlwaysOnTop(bool on) { setValue("graphWindow/alwaysOnTop", on); }

    constexpr bool graphWindowFramelessDefault() const { return false; }
    bool graphWindowFrameless() const { return valueBool("graphWindow/frameless"); }
    void setGraphWindowFrameless(bool on) { setValue("graphWindow/frameless", on); }

    constexpr bool graphWindowClickThroughDefault() const { return false; }
    bool graphWindowClickThrough() const { return valueBool("graphWindow/clickThrough"); }
    void setGraphWindowClickThrough(bool on) { setValue("graphWindow/clickThrough", on); }

    constexpr bool graphWindowHideOnHoverDefault() const { return false; }
    bool graphWindowHideOnHover() const { return valueBool("graphWindow/hideOnHover"); }
    void setGraphWindowHideOnHover(bool on) { setValue("graphWindow/hideOnHover", on); }

    constexpr int graphWindowOpacityDefault() const { return 90; }
    int graphWindowOpacity() const { return valueInt("graphWindow/opacity", 90); }
    void setGraphWindowOpacity(int v) { setValue("graphWindow/opacity", v); }

    constexpr int graphWindowHoverOpacityDefault() const { return 95; }
    int graphWindowHoverOpacity() const { return valueInt("graphWindow/hoverOpacity", 95); }
    void setGraphWindowHoverOpacity(int v) { setValue("graphWindow/hoverOpacity", v); }

    constexpr int graphWindowMaxSecondsDefault() const { return 500; }
    int graphWindowMaxSeconds() const { return valueInt("graphWindow/maxSeconds", 500); }
    void setGraphWindowMaxSeconds(int v) { setValue("graphWindow/maxSeconds", v); }

    constexpr int graphWindowFixedSpeedDefault() const { return 0; }
    int graphWindowFixedSpeed() const { return valueInt("graphWindow/fixedSpeed"); }
    void setGraphWindowFixedSpeed(int v) { setValue("graphWindow/fixedSpeed", v); }

    constexpr int graphWindowTrafUnitDefault() const { return 0; }
    int graphWindowTrafUnit() const { return valueInt("graphWindow/trafUnit", 0); }
    void setGraphWindowTrafUnit(int v) { setValue("graphWindow/trafUnit", v); }

    constexpr QColor graphWindowColorDefault() const { return QColor(255, 255, 255); }
    QColor graphWindowColor() const
    {
        return valueColor("graphWindow/color", graphWindowColorDefault());
    }
    void setGraphWindowColor(const QColor &v) { setColor("graphWindow/color", v); }

    constexpr QColor graphWindowColorInDefault() const { return QColor(52, 196, 84); }
    QColor graphWindowColorIn() const
    {
        return valueColor("graphWindow/colorIn", graphWindowColorInDefault());
    }
    void setGraphWindowColorIn(const QColor &v) { setColor("graphWindow/colorIn", v); }

    constexpr QColor graphWindowColorOutDefault() const { return QColor(235, 71, 63); }
    QColor graphWindowColorOut() const
    {
        return valueColor("graphWindow/colorOut", graphWindowColorOutDefault());
    }
    void setGraphWindowColorOut(const QColor &v) { setColor("graphWindow/colorOut", v); }

    constexpr QColor graphWindowAxisColorDefault() const { return QColor(108, 108, 108); }
    QColor graphWindowAxisColor() const
    {
        return valueColor("graphWindow/axisColor", graphWindowAxisColorDefault());
    }
    void setGraphWindowAxisColor(const QColor &v) { setColor("graphWindow/axisColor", v); }

    constexpr QColor graphWindowTickLabelColorDefault() const { return QColor(0, 0, 0); }
    QColor graphWindowTickLabelColor() const
    {
        return valueColor("graphWindow/tickLabelColor", graphWindowTickLabelColorDefault());
    }
    void setGraphWindowTickLabelColor(const QColor &v)
    {
        setColor("graphWindow/tickLabelColor", v);
    }

    constexpr QColor graphWindowLabelColorDefault() const { return QColor(33, 33, 33); }
    QColor graphWindowLabelColor() const
    {
        return valueColor("graphWindow/labelColor", graphWindowLabelColorDefault());
    }
    void setGraphWindowLabelColor(const QColor &v) { setColor("graphWindow/labelColor", v); }

    constexpr QColor graphWindowGridColorDefault() const { return QColor(141, 141, 141); }
    QColor graphWindowGridColor() const
    {
        return valueColor("graphWindow/gridColor", graphWindowGridColorDefault());
    }
    void setGraphWindowGridColor(const QColor &v) { setColor("graphWindow/gridColor", v); }

    constexpr QColor graphWindowDarkColorDefault() const { return QColor(56, 56, 56); }
    QColor graphWindowDarkColor() const
    {
        return valueColor("graphWindow/darkColor", graphWindowDarkColorDefault());
    }
    void setGraphWindowDarkColor(const QColor &v) { setColor("graphWindow/darkColor", v); }

    constexpr QColor graphWindowDarkColorInDefault() const { return QColor(52, 196, 84); }
    QColor graphWindowDarkColorIn() const
    {
        return valueColor("graphWindow/darkColorIn", graphWindowDarkColorInDefault());
    }
    void setGraphWindowDarkColorIn(const QColor &v) { setColor("graphWindow/darkColorIn", v); }

    constexpr QColor graphWindowDarkColorOutDefault() const { return QColor(235, 71, 63); }
    QColor graphWindowDarkColorOut() const
    {
        return valueColor("graphWindow/darkColorOut", graphWindowDarkColorOutDefault());
    }
    void setGraphWindowDarkColorOut(const QColor &v) { setColor("graphWindow/darkColorOut", v); }

    constexpr QColor graphWindowDarkAxisColorDefault() const { return QColor(141, 141, 141); }
    QColor graphWindowDarkAxisColor() const
    {
        return valueColor("graphWindow/darkAxisColor", graphWindowDarkAxisColorDefault());
    }
    void setGraphWindowDarkAxisColor(const QColor &v) { setColor("graphWindow/darkAxisColor", v); }

    constexpr QColor graphWindowDarkTickLabelColorDefault() const { return QColor(248, 248, 248); }
    QColor graphWindowDarkTickLabelColor() const
    {
        return valueColor("graphWindow/darkTickLabelColor", graphWindowDarkTickLabelColorDefault());
    }
    void setGraphWindowDarkTickLabelColor(const QColor &v)
    {
        setColor("graphWindow/darkTickLabelColor", v);
    }

    constexpr QColor graphWindowDarkLabelColorDefault() const { return QColor(248, 248, 248); }
    QColor graphWindowDarkLabelColor() const
    {
        return valueColor("graphWindow/darkLabelColor", graphWindowDarkLabelColorDefault());
    }
    void setGraphWindowDarkLabelColor(const QColor &v)
    {
        setColor("graphWindow/darkLabelColor", v);
    }

    constexpr QColor graphWindowDarkGridColorDefault() const { return QColor(108, 108, 108); }
    QColor graphWindowDarkGridColor() const
    {
        return valueColor("graphWindow/darkGridColor", graphWindowDarkGridColorDefault());
    }
    void setGraphWindowDarkGridColor(const QColor &v) { setColor("graphWindow/darkGridColor", v); }

    constexpr int graphWindowTickLabelSizeDefault() const { return 9; }
    int graphWindowTickLabelSize() const
    {
        return valueInt("graphWindow/tickLabelSize", graphWindowTickLabelSizeDefault());
    }
    void setGraphWindowTickLabelSize(int v) { setValue("graphWindow/tickLabelSize", v); }

    static QString statWindowGroup() { return "statWindow"; }

    QRect statWindowGeometry() const { return value("statWindow/geometry").toRect(); }
    void setStatWindowGeometry(const QRect &v) { setValue("statWindow/geometry", v); }

    bool statWindowMaximized() const { return valueBool("statWindow/maximized"); }
    void setStatWindowMaximized(bool on) { setValue("statWindow/maximized", on); }

    int statTabIndex() const { return valueInt("statWindow/tabIndex"); }
    void setStatTabIndex(int v) { setValue("statWindow/tabIndex", v); }

    int statTrafUnit() const { return valueInt("statWindow/trafUnit"); }
    void setStatTrafUnit(int v) { setValue("statWindow/trafUnit", v); }

    QByteArray statWindowTrafSplit() const { return valueByteArray("statWindow/trafSplit"); }
    void setStatWindowTrafSplit(const QByteArray &v) { setValue("statWindow/trafSplit", v); }

    int statAppListHeaderVersion() const { return valueInt("statWindow/appListHeaderVersion"); }
    void setStatAppListHeaderVersion(int v) { setValue("statWindow/appListHeaderVersion", v); }

    QByteArray statAppListHeader() const { return valueByteArray("statWindow/appListHeader"); }
    void setStatAppListHeader(const QByteArray &v) { setValue("statWindow/appListHeader", v); }

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

    static int colorSchemeByName(const QString &theme);
    static QString colorSchemeName(int colorScheme);

private:
    QString m_defaultLanguage;
};

#endif // INIUSER_H
