#include "fortsettings.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QSettings>

#include <fort_version.h>

#include "conf/addressgroup.h"
#include "conf/firewallconf.h"
#include "util/dateutil.h"
#include "util/envmanager.h"
#include "util/fileutil.h"
#include "util/startuputil.h"

namespace {

QString pathSlash(const QString &path)
{
    return FileUtil::pathSlash(FileUtil::absolutePath(path));
}

QString expandPath(const QString &path, EnvManager *envManager = nullptr)
{
    const auto expPath = envManager ? envManager->expandString(path) : path;
    return pathSlash(expPath);
}

}

FortSettings::FortSettings(QObject *parent) :
    QObject(parent),
    m_iniExists(false),
    m_noCache(false),
    m_isService(false),
    m_hasService(false),
    m_isWindowControl(false),
    m_passwordChecked(false),
    m_passwordUnlocked(0),
    m_bulkUpdating(false),
    m_bulkIniChanged(false)
{
}

QString FortSettings::appUpdatesUrl() const
{
    return APP_UPDATES_URL;
}

int FortSettings::appVersion() const
{
    return APP_VERSION;
}

void FortSettings::setupGlobal()
{
    // Use global settings from program's binary directory
    const QSettings settings(FileUtil::nativeAppFilePath() + ".ini", QSettings::IniFormat);

    // High-DPI scale factor rounding policy
    const auto dpiPolicy = settings.value("global/dpiPolicy").toString();
    if (!dpiPolicy.isEmpty()) {
        qputenv("QT_SCALE_FACTOR_ROUNDING_POLICY", dpiPolicy.toLatin1());
    }

    m_noCache = settings.value("global/noCache").toBool();
    m_defaultLanguage = settings.value("global/defaultLanguage").toString();
    m_profilePath = settings.value("global/profileDir").toString();
    m_statPath = settings.value("global/statDir").toString();
    m_logsPath = settings.value("global/logsDir").toString();
    m_cachePath = settings.value("global/cacheDir").toString();
}

void FortSettings::initialize(const QStringList &args, EnvManager *envManager)
{
    processArguments(args, envManager);
    setupIni();
}

void FortSettings::processArguments(const QStringList &args, EnvManager *envManager)
{
    QCommandLineParser parser;

    const QCommandLineOption profileOption(QStringList() << "p"
                                                         << "profile",
            "Directory to store settings.", "profile");
    parser.addOption(profileOption);

    const QCommandLineOption statOption("stat", "Directory to store statistics.", "stat");
    parser.addOption(statOption);

    const QCommandLineOption logsOption("logs", "Directory to store logs.", "logs");
    parser.addOption(logsOption);

    const QCommandLineOption cacheOption("cache", "Directory to store cache.", "cache");
    parser.addOption(cacheOption);

    const QCommandLineOption uninstallOption("u", "Uninstall booted provider and startup entries.");
    parser.addOption(uninstallOption);

    const QCommandLineOption noCacheOption("no-cache", "Don't use cache on disk.");
    parser.addOption(noCacheOption);

    const QCommandLineOption langOption("lang", "Default language.", "lang", "en");
    parser.addOption(langOption);

    const QCommandLineOption serviceOption("service", "Is running as a service?");
    parser.addOption(serviceOption);

    const QCommandLineOption windowControlOption(
            "w", "Control running instance's window by sending the command.");
    parser.addOption(windowControlOption);

    const QCommandLineOption controlOption(QStringList() << "c"
                                                         << "control",
            "Control running instance by executing the command.", "control");
    parser.addOption(controlOption);

    parser.addVersionOption();
    parser.addHelpOption();

    parser.process(args);

    // No Cache
    if (parser.isSet(noCacheOption)) {
        m_noCache = true;
    }

    // Default Language
    if (parser.isSet(langOption)) {
        m_defaultLanguage = parser.value(langOption);
    }

    // Is service
    m_isService = parser.isSet(serviceOption);

    // Profile Path
    if (parser.isSet(profileOption)) {
        m_profilePath = parser.value(profileOption);
    }
    if (m_profilePath.isEmpty()) {
        const auto appBinLocation = FileUtil::appBinLocation();
        const bool isPortable = FileUtil::fileExists(appBinLocation + "/README.portable");

        m_profilePath = pathSlash(isPortable
                        ? appBinLocation + "/Data"
                        : (hasService() ? expandPath("%ProgramData%\\" APP_NAME, envManager)
                                        : FileUtil::appConfigLocation()));
    } else {
        m_profilePath = expandPath(m_profilePath, envManager);
    }

    // Statistics Path
    if (parser.isSet(statOption)) {
        m_statPath = parser.value(statOption);
    }
    if (m_statPath.isEmpty()) {
        m_statPath = m_profilePath;
    } else {
        m_statPath = expandPath(m_statPath, envManager);
    }

    // Logs Path
    if (parser.isSet(logsOption)) {
        m_logsPath = parser.value(logsOption);
    }
    if (m_logsPath.isEmpty()) {
        m_logsPath = m_profilePath + "logs/";
    } else {
        m_logsPath = expandPath(m_logsPath, envManager);
    }

    // Cache Path
    if (parser.isSet(cacheOption)) {
        m_cachePath = parser.value(cacheOption);
    }
    if (m_cachePath.isEmpty()) {
        m_cachePath = m_profilePath + "cache/";
    } else {
        m_cachePath = expandPath(m_cachePath, envManager);
    }

    // Control command
    m_isWindowControl = parser.isSet(windowControlOption);
    m_controlCommand = parser.value(controlOption);

    // Other Arguments
    m_args = parser.positionalArguments();

    m_appArguments = args.mid(1);
}

void FortSettings::setupIni()
{
    const QString iniPath(profilePath() + (APP_BASE ".ini"));

    FileUtil::makePath(profilePath());
    FileUtil::makePath(statPath());
    FileUtil::makePath(logsPath());
    if (!noCache()) {
        FileUtil::makePath(cachePath());
    }

    m_iniExists = FileUtil::fileExists(iniPath);
    m_ini = new QSettings(iniPath, QSettings::IniFormat, this);

    migrateIniOnStartup();
}

void FortSettings::setErrorMessage(const QString &errorMessage)
{
    if (m_errorMessage != errorMessage) {
        m_errorMessage = errorMessage;
        emit errorMessageChanged();
    }
}

QString FortSettings::statFilePath() const
{
    return statPath() + (APP_BASE ".stat");
}

QString FortSettings::confFilePath() const
{
    return profilePath() + (APP_BASE ".config");
}

void FortSettings::setPasswordChecked(bool checked, bool unlocked)
{
    if (m_passwordChecked == checked)
        return;

    m_passwordChecked = checked;
    m_passwordUnlocked = unlocked;

    emit iniChanged();
}

void FortSettings::resetCheckedPassword(int unlocked)
{
    if (unlocked != 0 && unlocked != m_passwordUnlocked)
        return;

    setPasswordChecked(false, 0);
}

bool FortSettings::isPasswordRequired()
{
    return hasPassword() && !(m_passwordUnlocked != 0 && m_passwordChecked);
}

void FortSettings::readConfIni(FirewallConf &conf) const
{
    m_ini->beginGroup("confFlags");
    conf.setProvBoot(iniBool("provBoot"));
    conf.setFilterEnabled(iniBool("filterEnabled", true));
    conf.setFilterLocals(iniBool("filterLocals"));
    conf.setStopTraffic(iniBool("stopTraffic"));
    conf.setStopInetTraffic(iniBool("stopInetTraffic"));
    conf.setAllowAllNew(iniBool("allowAllNew"));
    conf.setLogBlocked(iniBool("logBlocked", true));
    conf.setLogStat(iniBool("logStat", true));
    conf.setLogStatNoFilter(iniBool("logStatNoFilter", true));
    conf.setLogAllowedIp(iniBool("logAllowedIp", false));
    conf.setLogBlockedIp(iniBool("logBlockedIp", false));
    conf.setAppBlockAll(iniBool("appBlockAll", true));
    conf.setAppAllowAll(iniBool("appAllowAll"));
    conf.setAppGroupBits(iniUInt("appGroupBits", DEFAULT_APP_GROUP_BITS));
    m_ini->endGroup();

    m_ini->beginGroup("stat");
    conf.setActivePeriodEnabled(iniBool("activePeriodEnabled"));
    conf.setActivePeriodFrom(DateUtil::reformatTime(iniText("activePeriodFrom")));
    conf.setActivePeriodTo(DateUtil::reformatTime(iniText("activePeriodTo")));
    conf.setMonthStart(iniInt("monthStart", DEFAULT_MONTH_START));
    conf.setTrafHourKeepDays(iniInt("trafHourKeepDays", DEFAULT_TRAF_HOUR_KEEP_DAYS));
    conf.setTrafDayKeepDays(iniInt("trafDayKeepDays", DEFAULT_TRAF_DAY_KEEP_DAYS));
    conf.setTrafMonthKeepMonths(iniInt("trafMonthKeepMonths", DEFAULT_TRAF_MONTH_KEEP_MONTHS));
    conf.setTrafUnit(iniInt("trafUnit"));
    conf.setAllowedIpKeepCount(iniInt("allowedIpKeepCount", DEFAULT_LOG_IP_KEEP_COUNT));
    conf.setBlockedIpKeepCount(iniInt("blockedIpKeepCount", DEFAULT_LOG_IP_KEEP_COUNT));
    m_ini->endGroup();

    m_ini->beginGroup("quota");
    conf.setQuotaDayMb(iniUInt("quotaDayMb"));
    conf.setQuotaMonthMb(iniUInt("quotaMonthMb"));
    m_ini->endGroup();
}

bool FortSettings::writeConfIni(const FirewallConf &conf)
{
    m_ini->beginGroup("confFlags");
    setIniValue("provBoot", conf.provBoot());
    setIniValue("filterEnabled", conf.filterEnabled());
    setIniValue("filterLocals", conf.filterLocals());
    setIniValue("stopTraffic", conf.stopTraffic());
    setIniValue("stopInetTraffic", conf.stopInetTraffic());
    setIniValue("allowAllNew", conf.allowAllNew());
    setIniValue("logBlocked", conf.logBlocked());
    setIniValue("logStat", conf.logStat());
    setIniValue("logStatNoFilter", conf.logStatNoFilter());
    setIniValue("logAllowedIp", conf.logAllowedIp());
    setIniValue("logBlockedIp", conf.logBlockedIp());
    setIniValue("appBlockAll", conf.appBlockAll());
    setIniValue("appAllowAll", conf.appAllowAll());
    setIniValue("appGroupBits", conf.appGroupBits(), DEFAULT_APP_GROUP_BITS);
    m_ini->endGroup();

    m_ini->beginGroup("stat");
    setIniValue("activePeriodEnabled", conf.activePeriodEnabled());
    setIniValue("activePeriodFrom", conf.activePeriodFrom());
    setIniValue("activePeriodTo", conf.activePeriodTo());
    setIniValue("monthStart", conf.monthStart(), DEFAULT_MONTH_START);
    setIniValue("trafHourKeepDays", conf.trafHourKeepDays(), DEFAULT_TRAF_HOUR_KEEP_DAYS);
    setIniValue("trafDayKeepDays", conf.trafDayKeepDays(), DEFAULT_TRAF_DAY_KEEP_DAYS);
    setIniValue("trafMonthKeepMonths", conf.trafMonthKeepMonths(), DEFAULT_TRAF_MONTH_KEEP_MONTHS);
    setIniValue("trafUnit", conf.trafUnit());
    setIniValue("allowedIpKeepCount", conf.allowedIpKeepCount());
    setIniValue("blockedIpKeepCount", conf.blockedIpKeepCount());
    m_ini->endGroup();

    m_ini->beginGroup("quota");
    setIniValue("quotaDayMb", conf.quotaDayMb());
    setIniValue("quotaMonthMb", conf.quotaMonthMb());
    m_ini->endGroup();

    migrateIniOnWrite();

    return iniSync();
}

void FortSettings::migrateIniOnStartup()
{
    const int version = iniVersion();
    if (version == appVersion())
        return;

#if 0
    // COMPAT: v3.0.0: Options Window
    if (version < 0x030000) {
        setCacheValue("optWindow/geometry", m_ini->value("window/geometry"));
        setCacheValue("optWindow/maximized", m_ini->value("window/maximized"));
        // Abandon "window/addrSplit" & "window/appsSplit"
    }
#endif
}

void FortSettings::migrateIniOnWrite()
{
    const int version = iniVersion();
    if (version == appVersion())
        return;

    setIniVersion(appVersion());

#if 0
    // COMPAT: v3.0.0: Options Window
    if (version < 0x030000) {
        removeIniKey("window");
        m_ini->setValue("optWindow/geometry", cacheValue("optWindow/geometry"));
        m_ini->setValue("optWindow/maximized", cacheValue("optWindow/maximized"));
    }
#endif
}

bool FortSettings::confMigrated() const
{
    const int version = iniVersion();
    if (version == appVersion())
        return false;

#if 0
    // COMPAT: v3.0.0
    if (version < 0x030000 && appVersion() >= 0x030000)
        return true;
#endif

    return false;
}

bool FortSettings::confCanMigrate(QString &viaVersion) const
{
    const int version = iniVersion();
    if (version == appVersion())
        return true;

    // COMPAT: v3.0.0
    if (version < 0x030000 && appVersion() > 0x030000) {
        viaVersion = "3.0.0";
        return false;
    }

    return true;
}

bool FortSettings::iniBool(const QString &key, bool defaultValue) const
{
    return iniValue(key, defaultValue).toBool();
}

int FortSettings::iniInt(const QString &key, int defaultValue) const
{
    return iniValue(key, defaultValue).toInt();
}

uint FortSettings::iniUInt(const QString &key, int defaultValue) const
{
    return iniValue(key, defaultValue).toUInt();
}

qreal FortSettings::iniReal(const QString &key, qreal defaultValue) const
{
    return iniValue(key, defaultValue).toReal();
}

QString FortSettings::iniText(const QString &key, const QString &defaultValue) const
{
    return iniValue(key, defaultValue).toString();
}

QStringList FortSettings::iniList(const QString &key) const
{
    return iniValue(key).toStringList();
}

QVariantMap FortSettings::iniMap(const QString &key) const
{
    return iniValue(key).toMap();
}

QByteArray FortSettings::iniByteArray(const QString &key) const
{
    return iniValue(key).toByteArray();
}

QColor FortSettings::iniColor(const QString &key, const QColor &defaultValue) const
{
    const QString text = iniText(key);
    if (text.isEmpty())
        return defaultValue;

    if (text.at(0).isDigit())
        return QColor::fromRgba(text.toUInt());

    return { text };
}

void FortSettings::setIniColor(const QString &key, const QColor &value, const QColor &defaultValue)
{
    setIniValue(key, value.name(), defaultValue.isValid() ? defaultValue.name() : QString());
}

QVariant FortSettings::iniValue(const QString &key, const QVariant &defaultValue) const
{
    if (key.isEmpty())
        return QVariant();

    // Try to load from cache
    const auto cachedValue = cacheValue(key);
    if (!cachedValue.isNull())
        return cachedValue;

    // Load from .ini
    const auto value = m_ini->value(key, defaultValue);

    // Save to cache
    setCacheValue(key, value);

    return value;
}

void FortSettings::setIniValue(
        const QString &key, const QVariant &value, const QVariant &defaultValue)
{
    const QVariant oldValue = iniValue(key, defaultValue);
    if (oldValue == value)
        return;

    // Save to .ini
    m_ini->setValue(key, value);

    // Save to cache
    setCacheValue(key, value);

    if (m_bulkUpdating) {
        m_bulkIniChanged = true;
    } else {
        emit iniChanged();
    }
}

QVariant FortSettings::cacheValue(const QString &key) const
{
    return m_cache.value(key);
}

void FortSettings::setCacheValue(const QString &key, const QVariant &value) const
{
    m_cache.insert(key, value);
}

void FortSettings::bulkUpdateBegin()
{
    Q_ASSERT(!m_bulkUpdating);

    m_bulkUpdating = true;
    m_bulkIniChanged = false;
}

void FortSettings::bulkUpdateEnd()
{
    Q_ASSERT(m_bulkUpdating);

    m_bulkUpdating = false;

    const bool doEmit = m_bulkIniChanged;
    m_bulkIniChanged = false;

    iniSync();

    if (doEmit) {
        emit iniChanged();
    }
}

void FortSettings::removeIniKey(const QString &key)
{
    m_ini->remove(key);
}

QStringList FortSettings::iniChildKeys(const QString &prefix) const
{
    m_ini->beginGroup(prefix);
    const QStringList list = m_ini->childKeys();
    m_ini->endGroup();
    return list;
}

bool FortSettings::iniSync()
{
    m_ini->sync();

    return m_ini->status() == QSettings::NoError;
}
