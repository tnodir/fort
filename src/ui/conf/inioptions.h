#ifndef INIOPTIONS_H
#define INIOPTIONS_H

#include <util/ini/mapsettings.h>

#define DEFAULT_FILTER_OFF_SECONDS     0 // Disabled
#define DEFAULT_AUTO_LEARN_SECONDS     60
#define DEFAULT_APP_GROUP_BITS         quint32(-1)
#define DEFAULT_MONTH_START            1
#define DEFAULT_TRAF_HOUR_KEEP_DAYS    90 // ~3 months
#define DEFAULT_TRAF_DAY_KEEP_DAYS     365 // ~1 year
#define DEFAULT_TRAF_MONTH_KEEP_MONTHS 36 // ~3 years
#define DEFAULT_LOG_CONN_KEEP_COUNT    10000

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

    int filterOffSeconds() const
    {
        return valueInt("conf/filterOffSeconds", DEFAULT_FILTER_OFF_SECONDS);
    }
    void setFilterOffSeconds(int v) { setValue("conf/filterOffSeconds", v); }

    int autoLearnSeconds() const
    {
        return valueInt("conf/autoLearnSeconds", DEFAULT_AUTO_LEARN_SECONDS);
    }
    void setAutoLearnSeconds(int v) { setValue("conf/autoLearnSeconds", v); }

    bool taskInfoListSet() const { return contains("task/infoList_"); }

    QVariant taskInfoList() const { return value("task/infoList_"); }
    void setTaskInfoList(const QVariant &v) { setValue("task/infoList_", v); }

    bool noServiceControl() const { return valueBool("protect/noServiceControl"); }
    void setNoServiceControl(bool v) { setValue("protect/noServiceControl", v); }

    bool disableCmdLine() const { return valueBool("protect/disableCmdLine"); }
    void setDisableCmdLine(bool v) { setValue("protect/disableCmdLine", v); }

    bool checkPasswordOnUninstallSet() const
    {
        return contains("protect/checkPasswordOnUninstall");
    }

    bool checkPasswordOnUninstall() const { return valueBool("protect/checkPasswordOnUninstall"); }
    void setCheckPasswordOnUninstall(bool v) { setValue("protect/checkPasswordOnUninstall", v); }

    static QString quotaDayAlertedKey() { return "quota/dayAlerted"; }
    static QString quotaMonthAlertedKey() { return "quota/monthAlerted"; }

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

    bool connClearOnExit() const { return valueBool("stat/connClearOnExit", true); }
    void setConnClearOnExit(bool v) { setValue("stat/connClearOnExit", v); }

    int connKeepCount() const
    {
        return valueInt("stat/connKeepCount", DEFAULT_LOG_CONN_KEEP_COUNT);
    }
    void setConnKeepCount(int v) { setValue("stat/connKeepCount", v); }

    bool updateKeepCurrentVersion() const { return valueBool("autoUpdate/keepCurrentVersion"); }
    void setUpdateKeepCurrentVersion(bool v) { setValue("autoUpdate/keepCurrentVersion", v); }

    bool updateAutoDownload() const { return valueBool("autoUpdate/autoDownload"); }
    void setUpdateAutoDownload(bool v) { setValue("autoUpdate/autoDownload", v); }

    bool updateAutoInstall() const { return valueBool("autoUpdate/autoInstall"); }
    void setUpdateAutoInstall(bool v) { setValue("autoUpdate/autoInstall", v); }

    bool progRemoveLearntApps() const { return valueBool("prog/removeLearntApps"); }
    void setProgRemoveLearntApps(bool v) { setValue("prog/removeLearntApps", v); }

    bool progPurgeOnMounted() const { return valueBool("prog/purgeOnMounted"); }
    void setProgPurgeOnMounted(bool v) { setValue("prog/purgeOnMounted", v); }
};

#endif // INIOPTIONS_H
