#ifndef INIOPTIONS_H
#define INIOPTIONS_H

#include "../util/ini/mapsettings.h"

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

    bool hasPassword() const { return valueBool("base/hasPassword_"); }
    void setHasPassword(bool v) { setValue("base/hasPassword_", v); }

    QString password() const { return valueText("base/password_"); }
    void setPassword(const QString &v) { setValue("base/password_", v); }

    bool explorerIntegratedSet() const { return contains("ext/explorerIntegrated_"); }
    void cacheExplorerIntegrated(bool v) { setCacheValue("ext/explorerIntegrated_", v); }

    bool explorerIntegrated() const { return valueBool("ext/explorerIntegrated_"); }
    void setExplorerIntegrated(bool v) { setValue("ext/explorerIntegrated_", v); }

    bool taskInfoListSet() const { return contains("task/infoList_"); }

    QVariant taskInfoList() const { return value("task/infoList_"); }
    void setTaskInfoList(const QVariant &v) { setValue("task/infoList_", v); }

    qint32 quotaDayAlerted() const { return valueInt("quota/dayAlerted"); }
    void setQuotaDayAlerted(qint32 v) { setValue("quota/dayAlerted", v); }

    qint32 quotaMonthAlerted() const { return valueInt("quota/monthAlerted"); }
    void setQuotaMonthAlerted(qint32 v) { setValue("quota/monthAlerted", v); }

    quint32 quotaDayMb() const { return valueUInt("quota/quotaDayMb"); }
    void setQuotaDayMb(quint32 v) { setValue("quota/quotaDayMb", v); }

    quint32 quotaMonthMb() const { return valueUInt("quota/quotaMonthMb"); }
    void setQuotaMonthMb(quint32 v) { setValue("quota/quotaMonthMMb", v); }

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

    int trafUnit() const { return valueInt("stat/trafUnit"); }
    void setTrafUnit(int v) { setValue("stat/trafUnit", v); }

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

    bool graphWindowAlwaysOnTop() const { return valueBool("graphWindow/alwaysOnTop", true); }
    void setGraphWindowAlwaysOnTop(bool on) { setValue("graphWindow/alwaysOnTop", on); }

    bool graphWindowFrameless() const { return valueBool("graphWindow/frameless"); }
    void setGraphWindowFrameless(bool on) { setValue("graphWindow/frameless", on); }

    bool graphWindowClickThrough() const { return valueBool("graphWindow/clickThrough"); }
    void setGraphWindowClickThrough(bool on) { setValue("graphWindow/clickThrough", on); }

    bool graphWindowHideOnHover() const { return valueBool("graphWindow/hideOnHover"); }
    void setGraphWindowHideOnHover(bool on) { setValue("graphWindow/hideOnHover", on); }

    int graphWindowOpacity() const { return valueInt("graphWindow/opacity", 90); }
    void setGraphWindowOpacity(int v) { setValue("graphWindow/opacity", v); }

    int graphWindowHoverOpacity() const { return valueInt("graphWindow/hoverOpacity", 95); }
    void setGraphWindowHoverOpacity(int v) { setValue("graphWindow/hoverOpacity", v); }

    int graphWindowMaxSeconds() const { return valueInt("graphWindow/maxSeconds", 500); }
    void setGraphWindowMaxSeconds(int v) { setValue("graphWindow/maxSeconds", v); }

    QColor graphWindowColor() const
    {
        return valueColor("graphWindow/color", QColor(255, 255, 255));
    }
    void setGraphWindowColor(const QColor &v) { setColor("graphWindow/color", v); }

    QColor graphWindowColorIn() const
    {
        return valueColor("graphWindow/colorIn", QColor(52, 196, 84));
    }
    void setGraphWindowColorIn(const QColor &v) { setColor("graphWindow/colorIn", v); }

    QColor graphWindowColorOut() const
    {
        return valueColor("graphWindow/colorOut", QColor(235, 71, 63));
    }
    void setGraphWindowColorOut(const QColor &v) { setColor("graphWindow/colorOut", v); }

    QColor graphWindowAxisColor() const
    {
        return valueColor("graphWindow/axisColor", QColor(0, 0, 0));
    }
    void setGraphWindowAxisColor(const QColor &v) { setColor("graphWindow/axisColor", v); }

    QColor graphWindowTickLabelColor() const
    {
        return valueColor("graphWindow/tickLabelColor", QColor(0, 0, 0));
    }
    void setGraphWindowTickLabelColor(const QColor &v)
    {
        setColor("graphWindow/tickLabelColor", v);
    }

    QColor graphWindowLabelColor() const
    {
        return valueColor("graphWindow/labelColor", QColor(0, 0, 0));
    }
    void setGraphWindowLabelColor(const QColor &v) { setColor("graphWindow/labelColor", v); }

    QColor graphWindowGridColor() const
    {
        return valueColor("graphWindow/gridColor", QColor(200, 200, 200));
    }
    void setGraphWindowGridColor(const QColor &v) { setColor("graphWindow/gridColor", v); }
};

#endif // INIOPTIONS_H
