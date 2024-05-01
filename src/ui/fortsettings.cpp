#include "fortsettings.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>

#include <fort_version.h>

#include <conf/firewallconf.h>
#include <manager/envmanager.h>
#include <util/dateutil.h>
#include <util/fileutil.h>
#include <util/osutil.h>
#include <util/startuputil.h>
#include <util/stringutil.h>

namespace {

bool g_isPortable = false;

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

FortSettings::FortSettings(QObject *parent) : Settings(parent) { }

QString FortSettings::confFilePath() const
{
    return profilePath() + APP_BASE + ".config";
}

QString FortSettings::statFilePath() const
{
    return statPath() + APP_BASE + ".stat";
}

QString FortSettings::statBlockFilePath() const
{
    return statFilePath() + "-block";
}

QString FortSettings::cacheFilePath() const
{
    return noCache() && !hasService() ? ":memory:" : cachePath() + "appinfo.db";
}

QString FortSettings::passwordUnlockedTillText() const
{
    if (passwordUnlockType() == UnlockDisabled)
        return QString();

    return unlockTypeStrings().at(passwordUnlockType());
}

void FortSettings::setPassword(const QString &password)
{
    setPasswordHash(StringUtil::cryptoHash(password));
}

bool FortSettings::checkPassword(const QString &password) const
{
    return StringUtil::cryptoHash(password) == passwordHash();
}

bool FortSettings::isPasswordRequired() const
{
    return hasPassword() && !passwordChecked();
}

void FortSettings::setPasswordChecked(bool checked, UnlockType unlockType)
{
    if (m_passwordChecked == checked && m_passwordUnlockType == unlockType)
        return;

    m_passwordChecked = checked;
    m_passwordUnlockType = checked ? unlockType : UnlockDisabled;

    emit passwordCheckedChanged();
}

void FortSettings::resetCheckedPassword(UnlockType unlockType)
{
    if (unlockType != UnlockDisabled && unlockType != m_passwordUnlockType)
        return;

    setPasswordChecked(false);
}

void FortSettings::setupGlobal()
{
    const QFileInfo appFileInfo(FileUtil::nativeAppFilePath());

    // Set working directory
    FileUtil::setCurrentDirectory(appFileInfo.path());

    // Is portable?
    g_isPortable = FileUtil::fileExists(appFileInfo.path() + "/README.portable");

    // Global settings from program's binary directory
    const QSettings settings(appFileInfo.filePath() + ".ini", QSettings::IniFormat);

    // High-DPI scale factor rounding policy
    const auto dpiPolicy = settings.value("global/dpiPolicy").toString();
    if (!dpiPolicy.isEmpty()) {
        qputenv("QT_SCALE_FACTOR_ROUNDING_POLICY", dpiPolicy.toLatin1());
    }

    m_hasService = StartupUtil::isServiceInstalled();
    m_isUserAdmin = OsUtil::isUserAdmin();

    m_noCache = settings.value("global/noCache").toBool();
    m_forceDebug = settings.value("global/forceDebug").toBool();
    m_canInstallDriver = settings.value("global/canInstallDriver").toBool();
    m_canStartService = settings.value("global/canStartService").toBool();
    m_defaultLanguage = settings.value("global/defaultLanguage").toString();

    m_profilePath = settings.value("global/profileDir").toString();
    m_statPath = settings.value("global/statDir").toString();
    m_cachePath = settings.value("global/cacheDir").toString();
    m_userPath = settings.value("global/userDir").toString();
    m_logsPath = settings.value("global/logsDir").toString();
}

void FortSettings::initialize(const QStringList &args, EnvManager *envManager)
{
    processArguments(args);

    setupPaths(envManager);
    createPaths();

    setupIni(profilePath() + APP_BASE + ".ini");
}

void FortSettings::processProfileOption(
        const QCommandLineParser &parser, const QCommandLineOption &profileOption)
{
    // Profile Path
    if (parser.isSet(profileOption)) {
        m_profilePath = parser.value(profileOption);
    }
}

void FortSettings::processStatOption(
        const QCommandLineParser &parser, const QCommandLineOption &statOption)
{
    // Statistics Path
    if (parser.isSet(statOption)) {
        m_statPath = parser.value(statOption);
    }
}

void FortSettings::processCacheOption(
        const QCommandLineParser &parser, const QCommandLineOption &cacheOption)
{
    // Cache Path
    if (parser.isSet(cacheOption)) {
        m_cachePath = parser.value(cacheOption);
    }
}

void FortSettings::processLogsOption(
        const QCommandLineParser &parser, const QCommandLineOption &logsOption)
{
    // Logs Path
    if (parser.isSet(logsOption)) {
        m_logsPath = parser.value(logsOption);
    }
}

void FortSettings::processNoCacheOption(
        const QCommandLineParser &parser, const QCommandLineOption &noCacheOption)
{
    // No Cache
    if (parser.isSet(noCacheOption)) {
        m_noCache = true;
    }
}

void FortSettings::processNoSplashOption(
        const QCommandLineParser &parser, const QCommandLineOption &noSplashOption)
{
    // No Splash
    if (parser.isSet(noSplashOption)) {
        m_noSplash = true;
    }
}

void FortSettings::processLangOption(
        const QCommandLineParser &parser, const QCommandLineOption &langOption)
{
    // Default Language
    if (parser.isSet(langOption)) {
        m_defaultLanguage = parser.value(langOption);
    }
}

void FortSettings::processRestartedOption(
        const QCommandLineParser &parser, const QCommandLineOption &restartedOption)
{
    // Restarted by Installer
    if (parser.isSet(restartedOption)) {
        m_isRestarted = true;
    }
}

void FortSettings::processServiceOption(
        const QCommandLineParser &parser, const QCommandLineOption &serviceOption)
{
    // Is service
    if (parser.isSet(serviceOption)) {
        m_isService = m_hasService = true;
    }
}

void FortSettings::processControlOption(
        const QCommandLineParser &parser, const QCommandLineOption &controlOption)
{
    // Control command
    m_controlCommand = parser.value(controlOption);
}

void FortSettings::processOtherOptions(const QCommandLineParser &parser)
{
    // Other Arguments
    m_args = parser.positionalArguments();
}

void FortSettings::processArguments(const QStringList &args)
{
    QCommandLineParser parser;

    const QCommandLineOption profileOption(
            QStringList() << "p" << "profile", "Directory to store settings.", "profile");
    parser.addOption(profileOption);

    const QCommandLineOption statOption("stat", "Directory to store statistics.", "stat");
    parser.addOption(statOption);

    const QCommandLineOption cacheOption("cache", "Directory to store cache.", "cache");
    parser.addOption(cacheOption);

    const QCommandLineOption logsOption("logs", "Directory to store logs.", "logs");
    parser.addOption(logsOption);

    const QCommandLineOption uninstallOption("u", "Uninstall boot filter and startup entries.");
    parser.addOption(uninstallOption);

    const QCommandLineOption installOption("i", "Install startup entries.");
    parser.addOption(installOption);

    const QCommandLineOption noCacheOption("no-cache", "Don't use cache on disk.");
    parser.addOption(noCacheOption);

    const QCommandLineOption noSplashOption("no-splash", "Don't show Splash screen on startup.");
    parser.addOption(noSplashOption);

    const QCommandLineOption langOption("lang", "Default language.", "lang", "en");
    parser.addOption(langOption);

    const QCommandLineOption restartedOption("restarted", "Restarted by Installer?");
    parser.addOption(restartedOption);

    const QCommandLineOption serviceOption("service", "Is running as a service?");
    parser.addOption(serviceOption);

    const QCommandLineOption controlOption(QStringList() << "c" << "control",
            "Control running instance by executing the command.", "control");
    parser.addOption(controlOption);

    parser.addVersionOption();
    parser.addHelpOption();

    parser.process(args);

    processProfileOption(parser, profileOption);
    processStatOption(parser, statOption);
    processCacheOption(parser, cacheOption);
    processLogsOption(parser, logsOption);
    processNoCacheOption(parser, noCacheOption);
    processNoSplashOption(parser, noSplashOption);
    processLangOption(parser, langOption);
    processRestartedOption(parser, restartedOption);
    processServiceOption(parser, serviceOption);
    processControlOption(parser, controlOption);
    processOtherOptions(parser);
}

void FortSettings::setupPaths(EnvManager *envManager)
{
    // Profile Path
    if (m_profilePath.isEmpty()) {
        m_isDefaultProfilePath = true;
        m_profilePath = defaultProfilePath(hasService());
    } else {
        m_profilePath = expandPath(m_profilePath, envManager);
    }

    // Statistics Path
    if (m_statPath.isEmpty()) {
        m_statPath = m_profilePath;
    } else {
        m_statPath = expandPath(m_statPath, envManager);
    }

    // Cache Path
    if (m_cachePath.isEmpty()) {
        m_cachePath = m_profilePath + "cache/";
    } else {
        m_cachePath = expandPath(m_cachePath, envManager);
    }

    // User Settings Path
    if (m_userPath.isEmpty()) {
        m_userPath = defaultProfilePath(isService());
    } else {
        m_userPath = expandPath(m_userPath, envManager);
    }

    // Logs Path
    if (m_logsPath.isEmpty()) {
        m_logsPath = m_userPath + "logs/";
    } else {
        m_logsPath = expandPath(m_logsPath, envManager);
    }
}

void FortSettings::createPaths()
{
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

    // Copy .ini to .user.ini
    // TODO: COMPAT: Remove after v4.1.0 (via v4.0.0)
    const QString iniUserPath = userPath() + APP_BASE + ".user.ini";
    if (!isService() && !FileUtil::fileExists(iniUserPath)) {
        FileUtil::copyFile(profilePath() + APP_BASE + ".ini", iniUserPath);
    }
}

bool FortSettings::isPortable()
{
    return g_isPortable;
}

QString FortSettings::defaultProfilePath(bool isService)
{
    // Is portable?
    if (isPortable()) {
        return FileUtil::appBinLocation() + "/Data/";
    }

    // Is service?
    if (isService) {
        const QString servicePath =
                FileUtil::expandPath(QLatin1String("%ProgramData%\\") + APP_NAME);
        return pathSlash(servicePath);
    }

    return pathSlash(FileUtil::appConfigLocation());
}

void FortSettings::readConfIni(FirewallConf &conf) const
{
    ini()->beginGroup("confFlags");
    conf.setBootFilter(iniBool("bootFilter"));
    conf.setFilterEnabled(iniBool("filterEnabled", true));
    conf.setFilterLocals(iniBool("filterLocals"));
    conf.setBlockTraffic(iniBool("blockTraffic"));
    conf.setBlockInetTraffic(iniBool("blockInetTraffic"));
    conf.setAllowAllNew(iniBool("allowAllNew", true));
    conf.setAskToConnect(iniBool("askToConnect"));
    conf.setLogStat(iniBool("logStat", true));
    conf.setLogStatNoFilter(iniBool("logStatNoFilter", true));
    conf.setLogBlocked(iniBool("logBlocked", true));
    conf.setLogAllowedIp(iniBool("logAllowedIp", true));
    conf.setLogBlockedIp(iniBool("logBlockedIp", true));
    conf.setLogAlertedBlockedIp(iniBool("logAlertedBlockedIp"));
    conf.setAppBlockAll(iniBool("appBlockAll", true));
    conf.setAppAllowAll(iniBool("appAllowAll"));
    conf.setupAppGroupBits(iniUInt("appGroupBits", DEFAULT_APP_GROUP_BITS));
    ini()->endGroup();

    ini()->beginGroup("stat");
    conf.setActivePeriodEnabled(iniBool("activePeriodEnabled"));
    conf.setActivePeriodFrom(DateUtil::reformatTime(iniText("activePeriodFrom")));
    conf.setActivePeriodTo(DateUtil::reformatTime(iniText("activePeriodTo")));
    ini()->endGroup();

    // Ini Options
    readConfIniOptions(conf.ini());
}

void FortSettings::readConfIniOptions(const IniOptions &ini) const
{
    // Check Password on Uninstall
    if (ini.checkPasswordOnUninstall()) {
        setCacheValue(passwordHashKey(), StartupUtil::registryPasswordHash());
    }
}

void FortSettings::writeConfIni(const FirewallConf &conf)
{
    bool changed = false;

    if (conf.flagsEdited()) {
        ini()->beginGroup("confFlags");
        setIniValue("bootFilter", conf.bootFilter());
        setIniValue("filterEnabled", conf.filterEnabled());
        setIniValue("filterLocals", conf.filterLocals());
        setIniValue("blockTraffic", conf.blockTraffic());
        setIniValue("blockInetTraffic", conf.blockInetTraffic());
        setIniValue("allowAllNew", conf.allowAllNew());
        setIniValue("askToConnect", conf.askToConnect());
        setIniValue("logStat", conf.logStat());
        setIniValue("logStatNoFilter", conf.logStatNoFilter());
        setIniValue("logBlocked", conf.logBlocked());
        setIniValue("logAllowedIp", conf.logAllowedIp());
        setIniValue("logBlockedIp", conf.logBlockedIp());
        setIniValue("logAlertedBlockedIp", conf.logAlertedBlockedIp());
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

    // Ini Options
    if (conf.iniEdited()) {
        writeConfIniOptions(conf.ini());

        changed = true;
    }

    if (changed) {
        iniFlush();
    }
}

void FortSettings::writeConfIniOptions(const IniOptions &ini)
{
    // Save changed keys
    ini.save();

    // Password
    const bool isPasswordSet = (ini.hasPasswordSet() && ini.hasPassword() != hasPassword());
    if (isPasswordSet || !ini.password().isEmpty()) {
        setPassword(ini.password());
    }

    // Check Password on Uninstall
    if (ini.checkPasswordOnUninstallSet() || ini.hasPasswordSet()) {
        StartupUtil::setRegistryPasswordHash(
                ini.checkPasswordOnUninstall() ? passwordHash() : QString());

        saveIniValue(passwordHashKey(), passwordHash());
    }
}

void FortSettings::migrateIniOnStartup()
{
    if (!iniExists()) {
        iniFlush();
        return;
    }

    int version;
    if (checkIniVersion(version))
        return;

    // COMPAT: v3.4.0
    if (version < 0x030400) {
        // Windows Explorer integration: Args changed
        if (StartupUtil::isExplorerIntegrated()) {
            StartupUtil::setExplorerIntegrated(false);
            StartupUtil::setExplorerIntegrated(true);
        }
    }

    // COMPAT: v3.8.1
    if (version < 0x030801) {
        setCacheValue("confFlags/bootFilter", ini()->value("confFlags/provBoot"));
    }

    // COMPAT: v3.8.4
    if (version < 0x030804) {
        setCacheValue("confFlags/askToConnect", false);
    }

    // COMPAT: v3.9.10
    if (version < 0x030910) {
        setCacheValue("confFlags/blockTraffic", ini()->value("confFlags/stopTraffic"));
        setCacheValue("confFlags/blockInetTraffic", ini()->value("confFlags/stopInetTraffic"));
        setCacheValue("quota/blockInetTraffic", ini()->value("quota/stopInetTraffic"));
    }
}

void FortSettings::migrateIniOnWrite()
{
    int version;
    if (checkIniVersionOnWrite(version))
        return;

    Settings::migrateIniOnWrite();

    // COMPAT: v3.4.0: .ini ~> .user.ini
    if (version < 0x030400) {
        removeIniKey("base/language");
        removeIniKey("hotKey");
        removeIniKey("stat/trafUnit");
        removeIniKey("graphWindow/visible");
        removeIniKey("graphWindow/geometry");
        removeIniKey("graphWindow/maximized");
        removeIniKey("optWindow");
        removeIniKey("progWindow");
        removeIniKey("zoneWindow");
        removeIniKey("connWindow");
    }

    // COMPAT: v3.8.1
    if (version < 0x030801) {
        removeIniKey("confFlags/provBoot");
        ini()->setValue("confFlags/bootFilter", cacheValue("confFlags/bootFilter"));
    }

    // COMPAT: v3.9.10
    if (version < 0x030910) {
        removeIniKey("confFlags/stopTraffic");
        removeIniKey("confFlags/stopInetTraffic");
        removeIniKey("quota/stopInetTraffic");
        ini()->setValue("confFlags/blockTraffic", cacheValue("confFlags/blockTraffic"));
        ini()->setValue("confFlags/blockInetTraffic", cacheValue("confFlags/blockInetTraffic"));
        ini()->setValue("quota/blockInetTraffic", cacheValue("quota/blockInetTraffic"));
    }
}

bool FortSettings::wasMigrated() const
{
    int version;
    if (checkIniVersion(version))
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
    int version;
    if (checkIniVersion(version))
        return true;

    // COMPAT: v3.0.0
    if (version < 0x030000 && appVersion() > 0x030000) {
        viaVersion = "3.0.0";
        return false;
    }

    return true;
}

QStringList FortSettings::unlockTypeStrings()
{
    return { tr("Window closed"), tr("Session lockout"), tr("Program exit") };
}
