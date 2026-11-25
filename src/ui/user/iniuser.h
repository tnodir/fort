#ifndef INIUSER_H
#define INIUSER_H

#include <util/ini/mapsettings.h>

namespace HotKey {

const char *const main = "main";
const char *const programs = "programs";
const char *const options = "options";
const char *const rules = "rules";
const char *const zones = "zones";
const char *const groups = "groups";
const char *const speedLimits = "speedLimits";
const char *const timePeriods = "timePeriods";
const char *const services = "services";
const char *const traffic = "traffic";
const char *const activity = "activity";
const char *const connections = "connections";
const char *const filterSim = "filterSim";
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
const char *const ruleModifier = "ruleModifier";
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

    QString styleSheetPath() const { return valueText("base/styleSheetPath"); }
    void setStyleSheetPath(const QString &v) { setValue("base/styleSheetPath", v); }

    static QString fontFamilyDefault() { return "Tahoma"; }
    QString fontFamily() const { return valueText("base/fontFamily", fontFamilyDefault()); }
    void setFontFamily(const QString &v) { setValue("base/fontFamily", v, fontFamilyDefault()); }

    static constexpr int fontSizeDefault() { return 9; }
    int fontSize() const { return valueInt("base/fontSize", fontSizeDefault()); }
    void setFontSize(int v) { setValue("base/fontSize", v, fontSizeDefault()); }

    static constexpr int listIconSizeDefault() { return 16; }
    int listIconSize() const { return valueInt("base/listIconSize", listIconSizeDefault()); }
    void setListIconSize(int v) { setValue("base/listIconSize", v, listIconSizeDefault()); }

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

    bool traySwitchWindow() const { return valueBool("tray/switchWindow"); }
    void setTraySwitchWindow(bool v) { setValue("tray/switchWindow", v); }

    bool confirmTrayFlags() const { return valueBool("confirm/trayFlags"); }
    void setConfirmTrayFlags(bool v) { setValue("confirm/trayFlags", v); }

    bool confirmQuit() const { return valueBool("confirm/quit", true); }
    void setConfirmQuit(bool v) { setValue("confirm/quit", v, true); }

    static QString mainWindowGroup() { return "mainWindow"; }

    QStringList mainWindowDocks() const
    {
        return value("mainWindow/docks", QStringList { activityWindowGroup() }).toStringList();
    }
    void setMainWindowDocks(const QStringList &v) { setValue("mainWindow/docks", v); }

    QByteArray mainWindowDocksState() const { return valueByteArray("mainWindow/docksState"); }
    void setMainWindowDocksState(const QByteArray &v) { setValue("mainWindow/docksState", v); }

    bool mainWindowAutoShowWindow() const { return valueBool("mainWindow/autoShowWindow", true); }
    void setMainWindowAutoShowWindow(bool v) { setValue("mainWindow/autoShowWindow", v, true); }

    static QString installationWindowGroup() { return "installationWindow"; }

    static QString progWindowGroup() { return "progWindow"; }

    bool progAppsSortDesc() const { return valueBool("progWindow/appsSortDesc"); }
    void setProgSortDesc(bool v) { setValue("progWindow/appsSortDesc", v); }

    int progAppsSortColumn() const { return valueInt("progWindow/appsSortColumn"); }
    void setProgSortColumn(int v) { setValue("progWindow/appsSortColumn", v); }

    int progAppsHeaderVersion() const { return valueInt("progWindow/appsHeaderVersion"); }
    void setProgAppsHeaderVersion(int v) { setValue("progWindow/appsHeaderVersion", v); }

    QByteArray progAppsHeader() const { return valueByteArray("progWindow/appsHeader"); }
    void setProgAppsHeader(const QByteArray &v) { setValue("progWindow/appsHeader", v); }

    int progAppsRowsHeight() const { return valueInt("progWindow/appsRowsHeight"); }
    void setProgAppsRowsHeight(int v) { setValue("progWindow/appsRowsHeight", v); }

    bool progWindowAutoClearAlerts() const { return valueBool("progWindow/autoClearAlerts"); }
    void setProgWindowAutoClearAlerts(bool on) { setValue("progWindow/autoClearAlerts", on); }

    int progWindowSortState() const { return valueInt("progWindow/sortState"); }
    void setProgWindowSortState(int v) { setValue("progWindow/sortState", v); }

    static QString progAlertWindowGroup() { return "progAlertWindow"; }

    static QString progConnWindowGroup() { return "progConnWindow"; }
    static QString progPendingConnWindowGroup() { return "progPendingConnWindow"; }
    static QString progAlertConnWindowGroup() { return "progAlertConnWindow"; }
    static QString progAlertPendingConnWindowGroup() { return "progAlertPendingConnWindow"; }

    // A connections window's state by its group
    QRect windowGeometry(const QString &group) const { return value(group + "/geometry").toRect(); }
    void setWindowGeometry(const QString &group, const QRect &v)
    {
        setValue(group + "/geometry", v);
    }

    bool windowMaximized(const QString &group) const { return valueBool(group + "/maximized"); }
    void setWindowMaximized(const QString &group, bool on) { setValue(group + "/maximized", on); }

    // The Activity window is docked by default
    bool windowDocked(const QString &group) const
    {
        return valueBool(group + "/docked", group == activityWindowGroup());
    }
    void setWindowDocked(const QString &group, bool on) { setValue(group + "/docked", on); }

    int windowConnListHeaderVersion(const QString &group) const
    {
        return valueInt(group + "/connListHeaderVersion");
    }
    void setWindowConnListHeaderVersion(const QString &group, int v)
    {
        setValue(group + "/connListHeaderVersion", v);
    }

    QByteArray windowConnListHeader(const QString &group) const
    {
        return valueByteArray(group + "/connListHeader");
    }
    void setWindowConnListHeader(const QString &group, const QByteArray &v)
    {
        setValue(group + "/connListHeader", v);
    }

    bool windowAutoScroll(const QString &group) const { return valueBool(group + "/autoScroll"); }
    void setWindowAutoScroll(const QString &group, bool on) { setValue(group + "/autoScroll", on); }

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

    QByteArray optWindowAddrSplit() const { return valueByteArray("optWindow/addrSplit"); }
    void setOptWindowAddrSplit(const QByteArray &v) { setValue("optWindow/addrSplit", v); }

    static QString ruleWindowGroup() { return "ruleWindow"; }

    int rulesHeaderVersion() const { return valueInt("ruleWindow/rulesHeaderVersion"); }
    void setRulesHeaderVersion(int v) { setValue("ruleWindow/rulesHeaderVersion", v); }

    QByteArray rulesHeader() const { return valueByteArray("ruleWindow/rulesHeader"); }
    void setRulesHeader(const QByteArray &v) { setValue("ruleWindow/rulesHeader", v); }

    int rulesExpanded() const { return valueInt("ruleWindow/rulesExpanded", 0x01); }
    void setRulesExpanded(int v) { setValue("ruleWindow/rulesExpanded", v); }

    static QString groupWindowGroup() { return "groupWindow"; }

    int groupsHeaderVersion() const { return valueInt("groupWindow/groupsHeaderVersion"); }
    void setGroupsHeaderVersion(int v) { setValue("groupWindow/groupsHeaderVersion", v); }

    QByteArray groupsHeader() const { return valueByteArray("groupWindow/groupsHeader"); }
    void setGroupsHeader(const QByteArray &v) { setValue("groupWindow/groupsHeader", v); }

    static QString speedLimitWindowGroup() { return "speedLimitWindow"; }

    int speedLimitsHeaderVersion() const
    {
        return valueInt("speedLimitWindow/speedLimitsHeaderVersion");
    }
    void setSpeedLimitsHeaderVersion(int v)
    {
        setValue("speedLimitWindow/speedLimitsHeaderVersion", v);
    }

    QByteArray speedLimitsHeader() const
    {
        return valueByteArray("speedLimitWindow/speedLimitsHeader");
    }
    void setSpeedLimitsHeader(const QByteArray &v)
    {
        setValue("speedLimitWindow/speedLimitsHeader", v);
    }

    static QString timePeriodWindowGroup() { return "timePeriodWindow"; }

    int timePeriodsHeaderVersion() const
    {
        return valueInt("timePeriodWindow/timePeriodsHeaderVersion");
    }
    void setTimePeriodsHeaderVersion(int v)
    {
        setValue("timePeriodWindow/timePeriodsHeaderVersion", v);
    }

    QByteArray timePeriodsHeader() const
    {
        return valueByteArray("timePeriodWindow/timePeriodsHeader");
    }
    void setTimePeriodsHeader(const QByteArray &v)
    {
        setValue("timePeriodWindow/timePeriodsHeader", v);
    }

    static QString serviceWindowGroup() { return "serviceWindow"; }

    int servicesHeaderVersion() const { return valueInt("serviceWindow/servicesHeaderVersion"); }
    void setServicesHeaderVersion(int v) { setValue("serviceWindow/servicesHeaderVersion", v); }

    QByteArray servicesHeader() const { return valueByteArray("serviceWindow/servicesHeader"); }
    void setServicesHeader(const QByteArray &v) { setValue("serviceWindow/servicesHeader", v); }

    static QString zoneWindowGroup() { return "zoneWindow"; }

    int zonesHeaderVersion() const { return valueInt("zoneWindow/zonesHeaderVersion"); }
    void setZonesHeaderVersion(int v) { setValue("zoneWindow/zonesHeaderVersion", v); }

    QByteArray zonesHeader() const { return valueByteArray("zoneWindow/zonesHeader"); }
    void setZonesHeader(const QByteArray &v) { setValue("zoneWindow/zonesHeader", v); }

    static QString filterSimWindowGroup() { return "filterSimWindow"; }

    static QString graphsWindowGroup() { return "graphsWindow"; }

    // The graphs' ids: the main graph's one (0) is always present
    QVector<int> graphWindowIds() const;

    // The new graph's id, -1 when the max count of the graphs is reached
    int newGraphWindowId() const;

    void addGraphWindowId(int graphId);
    // Removes the graph's options too
    void removeGraphWindowId(int graphId);

    int graphsHeaderVersion() const { return valueInt("graphsWindow/graphsHeaderVersion"); }
    void setGraphsHeaderVersion(int v) { setValue("graphsWindow/graphsHeaderVersion", v); }

    QByteArray graphsHeader() const { return valueByteArray("graphsWindow/graphsHeader"); }
    void setGraphsHeader(const QByteArray &v) { setValue("graphsWindow/graphsHeader", v); }

    static QString updatesWindowGroup() { return "updatesWindow"; }

    static QString statWindowGroup() { return "statWindow"; }

    static QString statTrafTabIndexKey() { return "statWindow/trafTabIndex"; }
    int statTrafTabIndex() const { return valueInt(statTrafTabIndexKey()); }
    void setStatTrafTabIndex(int v) { setValue(statTrafTabIndexKey(), v); }

    int statTrafUnit() const { return valueInt("statWindow/trafUnit"); }
    void setStatTrafUnit(int v) { setValue("statWindow/trafUnit", v); }

    QByteArray statWindowTrafSplit() const { return valueByteArray("statWindow/trafSplit"); }
    void setStatWindowTrafSplit(const QByteArray &v) { setValue("statWindow/trafSplit", v); }

    int statAppListHeaderVersion() const { return valueInt("statWindow/appListHeaderVersion"); }
    void setStatAppListHeaderVersion(int v) { setValue("statWindow/appListHeaderVersion", v); }

    QByteArray statAppListHeader() const { return valueByteArray("statWindow/appListHeader"); }
    void setStatAppListHeader(const QByteArray &v) { setValue("statWindow/appListHeader", v); }

    static QString connWindowGroup() { return "connWindow"; }

    static QString activityWindowGroup() { return "activityWindow"; }

    int activityHeaderVersion() const { return valueInt("activityWindow/headerVersion"); }
    void setActivityHeaderVersion(int v) { setValue("activityWindow/headerVersion", v); }

    QByteArray activityHeader() const { return valueByteArray("activityWindow/header"); }
    void setActivityHeader(const QByteArray &v) { setValue("activityWindow/header", v); }

public:
    void saveDefaultIni();

    static int colorSchemeByName(const QString &theme);
    static QString colorSchemeName(int colorScheme);

private:
    // The graphs' ids' bits: the main graph's one (0) is always set
    quint8 graphWindowIdsMask() const;
    void setGraphWindowIdsMask(quint8 v);

private:
    QString m_defaultLanguage;
};

#endif // INIUSER_H
