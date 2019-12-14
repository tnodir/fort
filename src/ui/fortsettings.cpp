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
    m_hasProvBoot(false),
    m_bulkUpdating(false),
    m_bulkIniChanged(false),
    m_ini(nullptr)
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

    const QCommandLineOption controlOption(
                QStringList() << "c" << "control",
                "Control running instance by executing the JS file.", "control");
    parser.addOption(controlOption);

    parser.addVersionOption();
    parser.addHelpOption();

    parser.process(args);

    // Portable Mode
    m_isPortable = FileUtil::fileExists(FileUtil::appBinLocation()
                                        + "/README.portable");

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

    // Control JS file path
    m_controlPath = parser.value(controlOption);

    // Other Arguments
    m_args = parser.positionalArguments();
}

void FortSettings::setupIni()
{
    const QString iniPath(profilePath() + "FortFirewall.ini");

    FileUtil::makePath(profilePath());
    FileUtil::makePath(statPath());

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

QString FortSettings::logsPath() const
{
    return profilePath() + QLatin1String("logs/");
}

QString FortSettings::statFilePath() const
{
    return statPath() + QLatin1String("FortFirewall.stat");
}

QString FortSettings::confFilePath() const
{
    return profilePath() + QLatin1String("FortFirewall.config");
}

QString FortSettings::confOldFilePath() const
{
    return profilePath() + QLatin1String("FortFirewall.conf");
}

QString FortSettings::confBackupFilePath() const
{
    return confOldFilePath() + QLatin1String(".backup");
}

bool FortSettings::readConf(FirewallConf &conf, bool &isNew)
{
    const QString filePath = confOldFilePath();
    const QString backupFilePath = confBackupFilePath();

    const bool fileExists = FileUtil::fileExists(filePath);
    const bool backupFileExists = FileUtil::fileExists(backupFilePath);

    isNew = !(fileExists || backupFileExists);

    return isNew || (fileExists && tryToReadConf(conf, filePath))
            || tryToReadConf(conf, backupFilePath);
}

bool FortSettings::tryToReadConf(FirewallConf &conf, const QString &filePath)
{
    const QByteArray data = FileUtil::readFileData(filePath);

    QJsonParseError jsonParseError{};
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(
                data, &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        setErrorMessage(jsonParseError.errorString());
        return false;
    }

    QVariant confVar = jsonDoc.toVariant();

    confVar = migrateConf(confVar);

    conf.fromVariant(confVar);

    return true;
}

bool FortSettings::writeConf(const FirewallConf &conf)
{
    const QString filePath = confOldFilePath();
    const QString backupFilePath = confBackupFilePath();

    if (!writeConfIni(conf)) {
        setErrorMessage(tr("Can't write .ini file"));
        return false;
    }

    const QJsonDocument jsonDoc = QJsonDocument::fromVariant(
                conf.toVariant());

    const QByteArray data = jsonDoc.toJson(QJsonDocument::Indented);

    if (!FileUtil::writeFileData(backupFilePath, data)) {
        setErrorMessage(tr("Can't create backup .conf file"));
        return false;
    }

    if (!FileUtil::writeFileData(filePath, data)) {
        setErrorMessage(tr("Can't create .conf file"));
        return false;
    }

    return true;
}

void FortSettings::readConfIni(FirewallConf &conf) const
{
    m_ini->beginGroup("confFlags");
    conf.setProvBoot(iniBool("provBoot"));
    conf.setFilterEnabled(iniBool("filterEnabled", true));
    conf.setFilterLocals(iniBool("filterLocals"));
    conf.setStopTraffic(iniBool("stopTraffic"));
    conf.setStopInetTraffic(iniBool("stopInetTraffic"));
    conf.setResolveAddress(iniBool("resolveAddress"));
    conf.setLogBlocked(iniBool("logBlocked"));
    conf.setLogStat(iniBool("logStat"));
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

QVariant FortSettings::migrateConf(const QVariant &confVar)
{
    const int version = iniVersion();
    if (version == appVersion())
        return confVar;

    QVariantMap map = confVar.toMap();

    // COMPAT: v1.7.0: AddressGroups
    if (version < 0x010700) {
        const QVariantMap oldIncMap = map["ipInclude"].toMap();
        const QVariantMap oldExcMap = map["ipExclude"].toMap();

        QVariantMap inetMap;

        inetMap["includeAll"] = iniBool("confFlags/ipIncludeAll");
        inetMap["excludeAll"] = iniBool("confFlags/ipExcludeAll");

        inetMap["includeText"] = oldIncMap["text"];
        inetMap["excludeText"] = oldExcMap["text"];

        QVariantList addrList;
        addrList.append(inetMap);

        map["addressGroups"] = addrList;
    }

    // COMPAT: v3.0.0: PasswordHash
    if (version < 0x030000) {
        const QString oldPasswordHash = map["passwordHash"].toString();

        setPasswordHash(oldPasswordHash);
    }

    return map;
}

void FortSettings::migrateIniOnStartup()
{
    const int version = iniVersion();
    if (version == appVersion())
        return;

    // COMPAT: v3.0.0: Options Window
    if (version < 0x030000) {
        setCacheValue("optWindow/geometry", m_ini->value("window/geometry"));
        setCacheValue("optWindow/maximized", m_ini->value("window/maximized"));
        setCacheValue("optWindow/addrSplit", m_ini->value("window/addrSplit"));
        setCacheValue("optWindow/appsSplit", m_ini->value("window/appsSplit"));
    }
}

void FortSettings::migrateIniOnWrite()
{
    const int version = iniVersion();
    if (version == appVersion())
        return;

    setIniVersion(appVersion());

    // COMPAT: v1.7.0: AddressGroups
    if (version < 0x010700) {
        removeIniKey("confFlags/ipIncludeAll");
        removeIniKey("confFlags/ipExcludeAll");
    }

    // COMPAT: v1.10.0: Log Errors
    if (version < 0x011000) {
        removeIniKey("confFlags/logErrors");
    }

    // COMPAT: v3.0.0: Options Window
    if (version < 0x030000) {
        removeIniKey("window");
        m_ini->setValue("optWindow/geometry", cacheValue("optWindow/geometry"));
        m_ini->setValue("optWindow/maximized", cacheValue("optWindow/maximized"));
        m_ini->setValue("optWindow/addrSplit", cacheValue("optWindow/addrSplit"));
        m_ini->setValue("optWindow/appsSplit", cacheValue("optWindow/appsSplit"));
    }
}

bool FortSettings::confMigrated() const
{
    const int version = iniVersion();
    if (version == appVersion())
        return false;

    // COMPAT: v3.0.0
    if (version < 0x030000 && appVersion() >= 0x030000)
        return true;

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
