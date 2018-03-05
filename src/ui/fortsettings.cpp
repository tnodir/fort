#include "fortsettings.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QSettings>

#include "conf/addressgroup.h"
#include "conf/firewallconf.h"
#include "util/fileutil.h"

FortSettings::FortSettings(const QStringList &args,
                           QObject *parent) :
    QObject(parent),
    m_hasProvBoot(false)
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

    parser.addVersionOption();
    parser.addHelpOption();

    parser.process(args);

    m_hasProvBoot = parser.isSet(provBootOption);

    m_profilePath = parser.value(profileOption);
    if (m_profilePath.isEmpty()) {
        m_profilePath = FileUtil::appConfigLocation();
    }
    m_profilePath = FileUtil::absolutePath(m_profilePath);

    const QLatin1Char slash('/');
    if (!m_profilePath.endsWith(slash)) {
        m_profilePath += slash;
    }
}

void FortSettings::setupIni()
{
    const QString iniPath(m_profilePath + "FortFirewall.ini");

    FileUtil::makePath(m_profilePath);

    m_ini = new QSettings(iniPath, QSettings::IniFormat, this);
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
    const QString tasksPrefix("tasks");

    foreach (const QString &taskName, iniChildKeys(tasksPrefix)) {
        const QString taskKey(tasksPrefix + '/' + taskName);
        map.insert(taskName, iniValue(taskKey).toByteArray());
    }
    return map;
}

bool FortSettings::setTasks(const TasksMap &map)
{
    const QString tasksPrefix("tasks");

    removeIniKey(tasksPrefix);

    foreach (const QString &taskName, map.keys()) {
        const QString taskKey(tasksPrefix + '/' + taskName);
        setIniValue(taskKey, map.value(taskName));
    }

    return iniSync();
}

QString FortSettings::logsPath() const
{
    return m_profilePath + QLatin1String("logs/");
}

QString FortSettings::statFilePath() const
{
    return m_profilePath + QLatin1String("FortFirewall.stat");
}

QString FortSettings::confFilePath() const
{
    return m_profilePath + QLatin1String("FortFirewall.conf");
}

QString FortSettings::confBackupFilePath() const
{
    return confFilePath() + QLatin1String(".backup");
}

bool FortSettings::readConf(FirewallConf &conf, bool &isNew)
{
    const QString filePath = confFilePath();
    const QString backupFilePath = confBackupFilePath();

    const bool fileExists = FileUtil::fileExists(filePath);
    const bool backupFileExists = FileUtil::fileExists(backupFilePath);

    isNew = !(fileExists || backupFileExists);

    return (isNew || (fileExists && tryToReadConf(conf, filePath))
            || tryToReadConf(conf, backupFilePath))
            && readConfIni(conf);  // read flags at the end to use correct app groups
}

bool FortSettings::tryToReadConf(FirewallConf &conf, const QString &filePath)
{
    const QByteArray data = FileUtil::readFileData(filePath);

    QJsonParseError jsonParseError;
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
    const QString filePath = confFilePath();
    const QString backupFilePath = confBackupFilePath();

    if (!writeConfIni(conf)) {
        setErrorMessage(tr("Can't write .ini file"));
        return false;
    }

    if (!tryToWriteConf(conf, backupFilePath)) {
        setErrorMessage(tr("Can't create backup .conf file"));
        return false;
    }

    if (!(FileUtil::removeFile(filePath)
          && FileUtil::copyFile(backupFilePath, filePath))) {
        setErrorMessage(tr("Can't create .conf file"));
        return false;
    }

    return true;
}

bool FortSettings::tryToWriteConf(const FirewallConf &conf, const QString &filePath)
{
    const QJsonDocument jsonDoc = QJsonDocument::fromVariant(
                conf.toVariant());

    const QByteArray data = jsonDoc.toJson(QJsonDocument::Indented);

    if (!FileUtil::writeFileData(filePath, data)) {
        setErrorMessage(tr("Can't write .conf file"));
        return false;
    }

    return true;
}

bool FortSettings::readConfIni(FirewallConf &conf) const
{
    m_ini->beginGroup("confFlags");
    conf.setProvBoot(iniBool("provBoot"));
    conf.setFilterEnabled(iniBool("filterEnabled", true));
    conf.setStopTraffic(iniBool("stopTraffic"));
    conf.setStopInetTraffic(iniBool("stopInetTraffic"));
    conf.setIgnoreTcpRst(iniBool("ignoreTcpRst"));
    conf.setResolveAddress(iniBool("resolveAddress"));
    conf.setLogBlocked(iniBool("logBlocked"));
    conf.setLogStat(iniBool("logStat"));
    conf.setAppBlockAll(iniBool("appBlockAll", true));
    conf.setAppAllowAll(iniBool("appAllowAll"));
    conf.setAppGroupBits(iniUInt("appGroupBits", DEFAULT_APP_GROUP_BITS));
    m_ini->endGroup();

    m_ini->beginGroup("stat");
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

    return true;
}

bool FortSettings::writeConfIni(const FirewallConf &conf)
{
    m_ini->beginGroup("confFlags");
    setIniValue("provBoot", conf.provBoot());
    setIniValue("filterEnabled", conf.filterEnabled());
    setIniValue("stopTraffic", conf.stopTraffic());
    setIniValue("stopInetTraffic", conf.stopInetTraffic());
    setIniValue("ignoreTcpRst", conf.ignoreTcpRst());
    setIniValue("resolveAddress", conf.resolveAddress());
    setIniValue("logBlocked", conf.logBlocked());
    setIniValue("logStat", conf.logStat());
    setIniValue("appBlockAll", conf.appBlockAll());
    setIniValue("appAllowAll", conf.appAllowAll());
    setIniValue("appGroupBits", conf.appGroupBits(), DEFAULT_APP_GROUP_BITS);
    m_ini->endGroup();

    m_ini->beginGroup("stat");
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

    removeMigratedKeys();

    return iniSync();
}

QVariant FortSettings::migrateConf(const QVariant &confVar)
{
    const int version = iniVersion();
    if (version == APP_VERSION)
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

    return map;
}

void FortSettings::removeMigratedKeys()
{
    const int version = iniVersion();
    if (version == APP_VERSION)
        return;

    setIniVersion(APP_VERSION);

    // COMPAT: v1.7.0: AddressGroups
    if (version < 0x010700) {
        removeIniKey("confFlags/ipIncludeAll");
        removeIniKey("confFlags/ipExcludeAll");
    }

    // COMPAT: v1.10.0: Log Errors
    if (version < 0x011000) {
        removeIniKey("confFlags/logErrors");
    }
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

QVariant FortSettings::iniValue(const QString &key,
                                const QVariant &defaultValue) const
{
    if (key.isEmpty())
        return QVariant();

    return m_ini->value(key, defaultValue);
}

void FortSettings::setIniValue(const QString &key, const QVariant &value,
                               const QVariant &defaultValue)
{
    if (m_ini->value(key, defaultValue) == value)
        return;

    m_ini->setValue(key, value);
    emit iniChanged();
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
