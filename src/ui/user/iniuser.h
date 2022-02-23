#ifndef INIUSER_H
#define INIUSER_H

#include <util/ini/mapsettings.h>

class IniUser : public MapSettings
{
public:
    explicit IniUser(Settings *settings = nullptr);

    QString defaultLanguage() const { return m_defaultLanguage; }
    void setDefaultLanguage(const QString &v) { m_defaultLanguage = v; }

    QString language() const { return valueText("base/language", defaultLanguage()); }
    void setLanguage(const QString &v) { setValue("base/language", v); }

    bool hotKeyEnabled() const { return valueBool("hotKey/enabled"); }
    void setHotKeyEnabled(bool v) { setValue("hotKey/enabled", v); }

    QString hotKeyPrograms() const { return valueText("hotKey/programs"); }
    QString hotKeyOptions() const { return valueText("hotKey/options"); }
    QString hotKeyStatistics() const { return valueText("hotKey/statistics"); }
    QString hotKeyGraph() const { return valueText("hotKey/graph"); }
    QString hotKeyZones() const { return valueText("hotKey/zones"); }
    QString hotKeyFilter() const { return valueText("hotKey/filter", "Ctrl+Alt+Shift+F"); }
    QString hotKeyStopTraffic() const { return valueText("hotKey/stopTraffic"); }
    QString hotKeyStopInetTraffic() const { return valueText("hotKey/stopInetTraffic"); }
    QString hotKeyAllowAllNew() const { return valueText("hotKey/allowAllNew"); }
    QString hotKeyAppGroupModifiers() const
    {
        return valueText("hotKey/appGroupModifiers", "Ctrl+Alt+Shift");
    }
    QString hotKeyQuit() const { return valueText("hotKey/quit"); }

    QString trayAction(const QString &event) const { return valueText("tray/" + event); }
    void setTrayAction(const QString &event, const QString &v) { setValue("tray/" + event, v); }

    bool confirmTrayFlags() const { return valueBool("confirm/trayFlags"); }
    void setConfirmTrayFlags(bool v) { setValue("confirm/trayFlags", v); }

    bool confirmQuit() const { return valueBool("confirm/quit"); }
    void setConfirmQuit(bool v) { setValue("confirm/quit", v); }

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

    QRect optWindowGeometry() const { return value("optWindow/geometry").toRect(); }
    void setOptWindowGeometry(const QRect &v) { setValue("optWindow/geometry", v); }

    bool optWindowMaximized() const { return valueBool("optWindow/maximized"); }
    void setOptWindowMaximized(bool on) { setValue("optWindow/maximized", on); }

    QByteArray optWindowAddrSplit() const { return valueByteArray("optWindow/addrSplit"); }
    void setOptWindowAddrSplit(const QByteArray &v) { setValue("optWindow/addrSplit", v); }

    QByteArray optWindowAppsSplit() const { return valueByteArray("optWindow/appsSplit"); }
    void setOptWindowAppsSplit(const QByteArray &v) { setValue("optWindow/appsSplit", v); }

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

private:
    QString m_defaultLanguage;
};

#endif // INIUSER_H
