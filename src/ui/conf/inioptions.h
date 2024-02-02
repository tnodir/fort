#ifndef INIOPTIONS_H
#define INIOPTIONS_H

#include <util/ini/mapsettings.h>

#define DEFAULT_APP_GROUP_BITS         0xFFFF
#define DEFAULT_MONTH_START            1
#define DEFAULT_TRAF_HOUR_KEEP_DAYS    90 // ~3 months
#define DEFAULT_TRAF_DAY_KEEP_DAYS     365 // ~1 year
#define DEFAULT_TRAF_MONTH_KEEP_MONTHS 36 // ~3 years
#define DEFAULT_LOG_IP_KEEP_COUNT      10000

class IniOptions : public MapSettings
{
public:
    explicit IniOptions(Settings *settings = nullptr);

    bool logDebug() const { return valueBool("base/debug"); }
    void setLogDebug(bool v) { setValue("base/debug", v); }

    bool logConsole() const { return valueBool("base/console"); }
    void setLogConsole(bool v) { setValue("base/console", v); }

    bool hasPasswordSet() const { return contains("base/hasPassword_"); }

    bool hasPassword() const { return valueBool("base/hasPassword_"); }
    void setHasPassword(bool v) { setValue("base/hasPassword_", v); }

    QString password() const { return valueText("base/password_"); }
    void setPassword(const QString &v) { setValue("base/password_", v); }

    bool taskInfoListSet() const { return contains("task/infoList_"); }

    QVariant taskInfoList() const { return value("task/infoList_"); }
    void setTaskInfoList(const QVariant &v) { setValue("task/infoList_", v); }

    bool noServiceControl() const { return valueBool("protect/noServiceControl"); }
    void setNoServiceControl(bool v) { setValue("protect/noServiceControl", v); }

    bool checkPasswordOnUninstallSet() const
    {
        return contains("protect/checkPasswordOnUninstall");
    }

    bool checkPasswordOnUninstall() const { return valueBool("protect/checkPasswordOnUninstall"); }
    void setCheckPasswordOnUninstall(bool v) { setValue("protect/checkPasswordOnUninstall", v); }

    int quotaDayAlerted() const { return valueInt("quota/dayAlerted"); }
    void setQuotaDayAlerted(int v) { setValue("quota/dayAlerted", v); }

    int quotaMonthAlerted() const { return valueInt("quota/monthAlerted"); }
    void setQuotaMonthAlerted(int v) { setValue("quota/monthAlerted", v); }

    int quotaDayMb() const { return valueInt("quota/quotaDayMb"); }
    void setQuotaDayMb(int v) { setValue("quota/quotaDayMb", v); }

    int quotaMonthMb() const { return valueInt("quota/quotaMonthMb"); }
    void setQuotaMonthMb(int v) { setValue("quota/quotaMonthMb", v); }

    bool quotaBlockInetTraffic() const { return valueBool("quota/blockInetTraffic"); }
    void setQuotaBlockInternet(bool v) { setValue("quota/blockInetTraffic", v); }

    int monthStart() const { return valueInt("stat/monthStart", DEFAULT_MONTH_START); }
    void setMonthStart(int v) { setValue("stat/monthStart", v); }

    int trafHourKeepDays() const
    {
        return valueInt("stat/trafHourKeepDays", DEFAULT_TRAF_HOUR_KEEP_DAYS);
    }
    void setTrafHourKeepDays(int v) { setValue("stat/trafHourKeepDays", v); }

    int trafDayKeepDays() const
    {
        return valueInt("stat/trafDayKeepDays", DEFAULT_TRAF_DAY_KEEP_DAYS);
    }
    void setTrafDayKeepDays(int v) { setValue("stat/trafDayKeepDays", v); }

    int trafMonthKeepMonths() const
    {
        return valueInt("stat/trafMonthKeepMonths", DEFAULT_TRAF_MONTH_KEEP_MONTHS);
    }
    void setTrafMonthKeepMonths(int v) { setValue("stat/trafMonthKeepMonths", v); }

    int allowedIpKeepCount() const
    {
        return valueInt("stat/allowedIpKeepCount", DEFAULT_LOG_IP_KEEP_COUNT);
    }
    void setAllowedIpKeepCount(int v) { setValue("stat/allowedIpKeepCount", v); }

    int blockedIpKeepCount() const
    {
        return valueInt("stat/blockedIpKeepCount", DEFAULT_LOG_IP_KEEP_COUNT);
    }
    void setBlockedIpKeepCount(int v) { setValue("stat/blockedIpKeepCount", v); }

    bool progPurgeOnMounted() const { return valueBool("prog/purgeOnMounted"); }
    void setProgPurgeOnMounted(bool v) { setValue("prog/purgeOnMounted", v); }

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

    constexpr QColor graphWindowAxisColorDefault() const { return QColor(0, 0, 0); }
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

    constexpr QColor graphWindowLabelColorDefault() const { return QColor(0, 0, 0); }
    QColor graphWindowLabelColor() const
    {
        return valueColor("graphWindow/labelColor", graphWindowLabelColorDefault());
    }
    void setGraphWindowLabelColor(const QColor &v) { setColor("graphWindow/labelColor", v); }

    constexpr QColor graphWindowGridColorDefault() const { return QColor(200, 200, 200); }
    QColor graphWindowGridColor() const
    {
        return valueColor("graphWindow/gridColor", graphWindowGridColorDefault());
    }
    void setGraphWindowGridColor(const QColor &v) { setColor("graphWindow/gridColor", v); }
};

#endif // INIOPTIONS_H
