#include "fortsettings.h"

#include <QCommandLineParser>
#include <QCoreApplication>

#include <fort_version.h>

#include "conf/firewallconf.h"
#include "util/dateutil.h"
#include "util/envmanager.h"
#include "util/fileutil.h"
#include "util/startuputil.h"
#include "util/stringutil.h"

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
    Settings(parent),
    m_isDefaultProfilePath(false),
    m_noCache(false),
    m_isService(false),
    m_hasService(false),
    m_isWindowControl(false),
    m_passwordChecked(false),
    m_passwordUnlockType(0)
{
}

QString FortSettings::confFilePath() const
{
    return profilePath() + APP_BASE + ".config";
}

QString FortSettings::statFilePath() const
{
    return statPath() + APP_BASE + ".stat";
}

QString FortSettings::cacheFilePath() const
{
    return noCache() ? ":memory:" : cachePath() + "appinfo.db";
}

void FortSettings::setPassword(const QString &password)
{
    setPasswordHash(StringUtil::cryptoHash(password));

    if (!hasPassword()) {
        resetCheckedPassword();
    }
}

bool FortSettings::checkPassword(const QString &password) const
{
    return StringUtil::cryptoHash(password) == passwordHash();
}

bool FortSettings::isPasswordRequired()
{
    return hasPassword() && !(m_passwordUnlockType != 0 && m_passwordChecked);
}

void FortSettings::setPasswordChecked(bool checked, int unlockType)
{
    if (m_passwordChecked == checked && m_passwordUnlockType == unlockType)
        return;

    m_passwordChecked = checked;
    m_passwordUnlockType = checked ? unlockType : 0;

    emit passwordCheckedChanged();
}

void FortSettings::resetCheckedPassword(int unlockType)
{
    if (unlockType != 0 && unlockType != m_passwordUnlockType)
        return;

    setPasswordChecked(false);
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
    m_userPath = settings.value("global/userDir").toString();
}

void FortSettings::initialize(const QStringList &args, EnvManager *envManager)
{
    processArguments(args);
    setupPaths(envManager);

    setupIni(profilePath() + APP_BASE + ".ini");

    if (isService()) {
        qputenv("FORT_SILENT", "1"); // For batch scripts
    }
}

void FortSettings::processArguments(const QStringList &args)
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
    if (parser.isSet(serviceOption)) {
        m_isService = m_hasService = true;
    }

    // Profile Path
    if (parser.isSet(profileOption)) {
        m_profilePath = parser.value(profileOption);
    }

    // Statistics Path
    if (parser.isSet(statOption)) {
        m_statPath = parser.value(statOption);
    }

    // Logs Path
    if (parser.isSet(logsOption)) {
        m_logsPath = parser.value(logsOption);
    }

    // Cache Path
    if (parser.isSet(cacheOption)) {
        m_cachePath = parser.value(cacheOption);
    }

    // Control command
    m_isWindowControl = parser.isSet(windowControlOption);
    m_controlCommand = parser.value(controlOption);

    // Other Arguments
    m_args = parser.positionalArguments();

    m_appArguments = args.mid(1);
}

void FortSettings::setupPaths(EnvManager *envManager)
{
    // Profile Path
    if (m_profilePath.isEmpty()) {
        m_isDefaultProfilePath = true;
        m_profilePath = defaultProfilePath(hasService(), envManager);
    } else {
        m_profilePath = expandPath(m_profilePath, envManager);
    }

    // Statistics Path
    if (m_statPath.isEmpty()) {
        m_statPath = m_profilePath;
    } else {
        m_statPath = expandPath(m_statPath, envManager);
    }

    // Logs Path
    if (m_logsPath.isEmpty()) {
        m_logsPath = m_profilePath + "logs/";
    } else {
        m_logsPath = expandPath(m_logsPath, envManager);
    }

    // Cache Path
    if (m_cachePath.isEmpty()) {
        m_cachePath = m_profilePath + "cache/";
    } else {
        m_cachePath = expandPath(m_cachePath, envManager);
    }

    // User Settings Path
    if (m_userPath.isEmpty()) {
        m_userPath = defaultConfigPath();
    } else {
        m_userPath = expandPath(m_userPath, envManager);
    }

    // Create directories
    FileUtil::makePath(profilePath());
    FileUtil::makePath(statPath());
    FileUtil::makePath(logsPath());
    if (!noCache()) {
        FileUtil::makePath(cachePath());
    }
    FileUtil::makePath(userPath());

    // Remove old cache file
    // TODO: COMPAT: Remove after v4.1.0 (via v4.0.0)
    FileUtil::removeFile(cachePath() + "appinfocache.db");
}

QString FortSettings::defaultProfilePath(bool hasService, EnvManager *envManager)
{
    // Is portable?
    {
        const auto appBinLocation = FileUtil::appBinLocation();
        const bool isPortable = FileUtil::fileExists(appBinLocation + "/README.portable");
        if (isPortable)
            return appBinLocation + "/Data/";
    }

    if (hasService)
        return expandPath(QLatin1String("%ProgramData%\\") + APP_NAME, envManager);

    return defaultConfigPath();
}

QString FortSettings::defaultConfigPath()
{
    return pathSlash(FileUtil::appConfigLocation());
}

void FortSettings::readConfIni(FirewallConf &conf) const
{
    ini()->beginGroup("confFlags");
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
    ini()->endGroup();

    ini()->beginGroup("stat");
    conf.setActivePeriodEnabled(iniBool("activePeriodEnabled"));
    conf.setActivePeriodFrom(DateUtil::reformatTime(iniText("activePeriodFrom")));
    conf.setActivePeriodTo(DateUtil::reformatTime(iniText("activePeriodTo")));
    ini()->endGroup();
}

void FortSettings::writeConfIni(const FirewallConf &conf)
{
    bool changed = false;

    if (conf.flagsEdited()) {
        ini()->beginGroup("confFlags");
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
        ini()->endGroup();

        ini()->beginGroup("stat");
        setIniValue("activePeriodEnabled", conf.activePeriodEnabled());
        setIniValue("activePeriodFrom", conf.activePeriodFrom());
        setIniValue("activePeriodTo", conf.activePeriodTo());
        ini()->endGroup();

        changed = true;
    }

    if (conf.iniEdited()) {
        const IniOptions &ini = conf.ini();

        // Save changed keys
        ini.save();

        // Password
        if (ini.hasPassword() != hasPassword() || !ini.password().isEmpty()) {
            setPassword(ini.password());
        }

        changed = true;
    }

    if (changed) {
        migrateIniOnWrite();
        iniSync();
    }
}

void FortSettings::migrateIniOnStartup()
{
    const int version = iniVersion();
    if (version == appVersion())
        return;

#if 0
    // COMPAT: v3.0.0: Options Window
    if (version < 0x030000) {
        setCacheValue("optWindow/geometry", ini()->value("window/geometry"));
        setCacheValue("optWindow/maximized", ini()->value("window/maximized"));
        // Abandon "window/addrSplit" & "window/appsSplit"
    }
#endif
}

void FortSettings::migrateIniOnWrite()
{
    const int version = iniVersion();
    if (version == appVersion())
        return;

    Settings::migrateIniOnWrite();

#if 0
    // COMPAT: v3.0.0: Options Window
    if (version < 0x030000) {
        removeIniKey("window");
        ini()->setValue("optWindow/geometry", cacheValue("optWindow/geometry"));
        ini()->setValue("optWindow/maximized", cacheValue("optWindow/maximized"));
    }
#endif
}

bool FortSettings::wasMigrated() const
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

bool FortSettings::canMigrate(QString &viaVersion) const
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
