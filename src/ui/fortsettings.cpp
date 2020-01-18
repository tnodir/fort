#include "fortsettings.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QSettings>

#include "conf/addressgroup.h"
#include "conf/firewallconf.h"
#include "util/dateutil.h"
#include "util/fileutil.h"

FortSettings::FortSettings(const QStringList &args,
                           QObject *parent) :
    QObject(parent),
    m_iniExists(false),
    m_isPortable(false),
    m_noCache(false),
    m_hasProvBoot(false),
    m_bulkUpdating(false),
    m_bulkIniChanged(false)
{
    processArguments(args);
    setupIni();
}

bool FortSettings::startWithWindows() const
{
    return FileUtil::fileExists(startupShortcutPath());
}

void FortSettings::setStartWithWindows(bool start)
{
    if (start == startWithWindows())
        return;

    const QString linkPath = startupShortcutPath();
    if (start) {
        FileUtil::linkFile(qApp->applicationFilePath(), linkPath);
    } else {
        FileUtil::removeFile(linkPath);
    }
    emit startWithWindowsChanged();
}

void FortSettings::processArguments(const QStringList &args)
{
    QCommandLineParser parser;

    const QCommandLineOption provBootOption(
                QStringList() << "b" << "boot",
                "Unblock access to network when Fort Firewall is not running.", "boot");
    parser.addOption(provBootOption);

    const QCommandLineOption profileOption(
                QStringList() << "p" << "profile",
                "Directory to store settings.", "profile");
    parser.addOption(profileOption);

    const QCommandLineOption statOption(
                QStringList() << "s" << "stat",
                "Directory to store statistics.", "stat");
    parser.addOption(statOption);

    const QCommandLineOption logsOption(
                QStringList() << "logs",
                "Directory to store logs.", "logs");
    parser.addOption(logsOption);

    const QCommandLineOption cacheOption(
                QStringList() << "cache",
                "Directory to store cache.", "cache");
    parser.addOption(cacheOption);

    const QCommandLineOption noCacheOption(
                QStringList() << "no-cache",
                "Don't use cache on disk.");
    parser.addOption(noCacheOption);

    const QCommandLineOption controlOption(
                QStringList() << "c" << "control",
                "Control running instance by executing the command.", "control");
    parser.addOption(controlOption);

    parser.addVersionOption();
    parser.addHelpOption();

    parser.process(args);

    // Portable Mode
    m_isPortable = FileUtil::fileExists(FileUtil::appBinLocation()
                                        + "/README.portable");

    // No Cache
    m_noCache = parser.isSet(noCacheOption);

    // Provider Boot
    m_hasProvBoot = parser.isSet(provBootOption);

    // Profile Path
    m_profilePath = parser.value(profileOption);
    if (m_profilePath.isEmpty()) {
        m_profilePath = m_isPortable
                ? FileUtil::appBinLocation() + "/Data"
                : FileUtil::appConfigLocation();
    }
    m_profilePath = FileUtil::pathSlash(
                FileUtil::absolutePath(m_profilePath));

    // Statistics Path
    m_statPath = parser.value(statOption);
    if (m_statPath.isEmpty()) {
        m_statPath = m_profilePath;
    } else {
        m_statPath = FileUtil::pathSlash(
                    FileUtil::absolutePath(m_statPath));
    }

    // Logs Path
    m_logsPath = parser.value(logsOption);
    if (m_logsPath.isEmpty()) {
        m_logsPath = m_profilePath + "logs/";
    } else {
        m_logsPath = FileUtil::pathSlash(
                    FileUtil::absolutePath(m_logsPath));
    }

    // Cache Path
    m_cachePath = parser.value(cacheOption);
    if (m_cachePath.isEmpty()) {
        m_cachePath = m_profilePath + "cache/";
    } else {
        m_cachePath = FileUtil::pathSlash(
                    FileUtil::absolutePath(m_cachePath));
    }

    // Control command
    m_controlCommand = parser.value(controlOption);

    // Other Arguments
    m_args = parser.positionalArguments();
}

void FortSettings::setupIni()
{
    const QString iniPath(profilePath() + "FortFirewall.ini");

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

TasksMap FortSettings::tasks() const
{
    TasksMap map;

    for (const QString &taskName : iniChildKeys(tasksKey())) {
        const QString taskKey(tasksKey() + '/' + taskName);
        map.insert(taskName, iniValue(taskKey).toByteArray());
    }
    return map;
}

bool FortSettings::setTasks(const TasksMap &map)
{
    removeTasks();

    auto keyIt = map.keyBegin();
    for (; keyIt != map.keyEnd(); ++keyIt) {
        const QString taskName = *keyIt;
        const QString taskKey(tasksKey() + '/' + taskName);
        setIniValue(taskKey, map.value(taskName));
    }

    return iniSync();
}

void FortSettings::removeTasks()
{
    removeIniKey(tasksKey());
}

QString FortSettings::statFilePath() const
{
    return statPath() + QLatin1String("FortFirewall.stat");
}

QString FortSettings::confFilePath() const
{
    return profilePath() + QLatin1String("FortFirewall.config");
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
    conf.setResolveAddress(iniBool("resolveAddress"));
    conf.setLogBlocked(iniBool("logBlocked", true));
    conf.setLogStat(iniBool("logStat", true));
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
    setIniValue("resolveAddress", conf.resolveAddress());
    setIniValue("logBlocked", conf.logBlocked());
    setIniValue("logStat", conf.logStat());
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

    return {text};
}

void FortSettings::setIniColor(const QString &key, const QColor &value,
                               const QColor &defaultValue)
{
    setIniValue(key, value.name(),
                defaultValue.isValid() ? defaultValue.name() : QString());
}

QVariant FortSettings::iniValue(const QString &key,
                                const QVariant &defaultValue) const
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

void FortSettings::setIniValue(const QString &key, const QVariant &value,
                               const QVariant &defaultValue)
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

QString FortSettings::startupShortcutPath()
{
    return FileUtil::applicationsLocation() + QLatin1Char('\\')
            + "Startup" + QLatin1Char('\\')
            + qApp->applicationName() + ".lnk";
}
